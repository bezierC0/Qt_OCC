#include "SheetMetalCollisionChecker.h"
#include "SheetMetalConnectorBuilder.h"
#include "SheetMetalFaceGeometry.h"
#include "SheetMetalConnectorMetadata.h"
#include "SheetMetalObstacleMap.h"
#include "SheetMetalRouteEvaluator.h"
#include "SheetMetalRoutePlanner.h"

#include <AIS_Shape.hxx>
#include <BRepAlgoAPI_Common.hxx>
#include <BRepBndLib.hxx>
#include <BRepCheck_Analyzer.hxx>
#include <BRepExtrema_DistShapeShape.hxx>
#include <BRepGProp.hxx>
#include <BRepPrimAPI_MakeBox.hxx>
#include <BRep_Tool.hxx>
#include <BRepClass_FaceClassifier.hxx>
#include <Bnd_Box.hxx>
#include <GProp_GProps.hxx>
#include <Precision.hxx>
#include <TopAbs_ShapeEnum.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopLoc_Location.hxx>

#include <cmath>
#include <iostream>
#include <vector>

namespace
{
bool containsSolid(const TopoDS_Shape& shape)
{
    TopExp_Explorer explorer(shape, TopAbs_SOLID);
    return explorer.More();
}

bool require(bool condition, const char* message)
{
    if (!condition) {
        std::cerr << message << '\n';
    }
    return condition;
}

SheetMetalConnectorData makeData(const gp_Pnt& start,
                                 const gp_Dir& startNormal,
                                 const gp_Pnt& end,
                                 const gp_Dir& endNormal)
{
    SheetMetalConnectorData data;
    data.start.position = start;
    data.start.normal = startNormal;
    data.end.position = end;
    data.end.normal = endNormal;
    data.width = 20.0;
    data.thickness = 2.0;
    data.clearance = 1.0;
    data.upDirection = startNormal;
    return data;
}

SheetMetalRoutePoint makeRoutePoint(const gp_Pnt& position,
                                    double bendRadius,
                                    bool isEndpoint = false)
{
    SheetMetalRoutePoint point;
    point.position = position;
    point.transition.bend.radius = bendRadius;
    point.isEndpoint = isEndpoint;
    return point;
}
}

int main()
{
    bool success = true;

    const TopoDS_Shape centerTestBox = BRepPrimAPI_MakeBox(
        gp_Pnt(20.0, 30.0, 40.0), 10.0, 20.0, 30.0).Shape();
    TopExp_Explorer faceExplorer(centerTestBox, TopAbs_FACE);
    gp_Pnt facePoint;
    gp_Dir faceNormal;
    success &= require(faceExplorer.More(), "Center test box has no face.");
    if (faceExplorer.More()) {
        const TopoDS_Face face = TopoDS::Face(faceExplorer.Current());
        success &= require(
            SheetMetalFaceGeometry::centerPointAndNormal(
                face, facePoint, faceNormal),
            "Face center evaluation failed.");
        BRepClass_FaceClassifier classifier(
            face, facePoint, Precision::Confusion());
        success &= require(
            classifier.State() == TopAbs_IN || classifier.State() == TopAbs_ON,
            "Evaluated face center is outside the face.");

        gp_Trsf translation;
        translation.SetTranslation(gp_Vec(100.0, 200.0, 300.0));
        const TopoDS_Face movedFace = TopoDS::Face(
            face.Moved(TopLoc_Location(translation)));
        gp_Pnt movedPoint;
        gp_Dir movedNormal;
        success &= require(
            SheetMetalFaceGeometry::centerPointAndNormal(
                movedFace, movedPoint, movedNormal),
            "Transformed face center evaluation failed.");
        success &= require(
            movedPoint.Distance(facePoint.Translated(gp_Vec(100.0, 200.0, 300.0)))
                <= Precision::Confusion(),
            "Transformed face center is not in world coordinates.");
    }

    auto straight = makeData(
        gp_Pnt(0.0, 0.0, 0.0), gp_Dir(0.0, 0.0, 1.0),
        gp_Pnt(100.0, 0.0, 0.0), gp_Dir(0.0, 0.0, -1.0));
    QString errorMessage;
    success &= require(
        SheetMetalRoutePlanner::computeOrthogonalRoute(straight, 5.0, &errorMessage),
        "Straight route planning failed.");
    success &= require(straight.routePoints.size() == 2,
                       "Straight route contains unexpected waypoints.");

    auto localPlaneRoute = makeData(
        gp_Pnt(0.0, 0.0, 0.0), gp_Dir(0.0, 0.0, 1.0),
        gp_Pnt(100.0, 100.0, 50.0), gp_Dir(-1.0, -1.0, 0.0));
    int checkedCandidateCount = 0;
    const auto acceptLocalPlaneRoute = [&checkedCandidateCount](
        const SheetMetalConnectorData& candidate) {
        ++checkedCandidateCount;
        return candidate.routePoints.size() == 3;
    };
    success &= require(
        SheetMetalRoutePlanner::computeOrthogonalRoute(
            localPlaneRoute, 12.0, &errorMessage, acceptLocalPlaneRoute),
        "Start-face plane route planning failed.");
    success &= require(
        checkedCandidateCount == 1
            && localPlaneRoute.routePoints.size() == 3
            && localPlaneRoute.routePoints[1].position.Distance(
                   gp_Pnt(100.0, 100.0, 0.0)) <= Precision::Confusion(),
        "The intermediate route point is not on the start-face plane.");

    auto invalidStartOrientation = makeData(
        gp_Pnt(0.0, 0.0, 0.0), gp_Dir(1.0, 0.0, 0.0),
        gp_Pnt(100.0, 0.0, 0.0), gp_Dir(-1.0, 0.0, 0.0));
    success &= require(
        !SheetMetalRoutePlanner::computeOrthogonalRoute(
            invalidStartOrientation, 5.0, &errorMessage),
        "A route leaving along the start normal was accepted.");

    std::vector<gp_Pnt> straightPoints;
    for (const auto& routePoint : straight.routePoints) {
        straightPoints.push_back(routePoint.position);
    }
    const auto straightEvaluation = SheetMetalRouteEvaluator::evaluate(
        straightPoints, straight);
    success &= require(straightEvaluation.isValid
                           && straightEvaluation.bendCount == 0
                           && std::abs(straightEvaluation.length - 100.0)
                               <= Precision::Confusion(),
                       "Straight route evaluation is incorrect.");

    auto invalidPoints = straightPoints;
    invalidPoints.insert(invalidPoints.begin() + 1, invalidPoints.front());
    const auto invalidEvaluation = SheetMetalRouteEvaluator::evaluate(
        invalidPoints, straight);
    success &= require(!invalidEvaluation.isValid
                           && !invalidEvaluation.failureReasons.isEmpty(),
                       "Invalid route evaluation did not report a failure reason.");

    const auto straightBuild = SheetMetalConnectorBuilder::build(straight);
    success &= require(straightBuild.isValid(), "Straight connector build failed.");
    success &= require(containsSolid(straightBuild.solid),
                       "Straight connector does not contain a solid.");

    auto twistedEnd = straight;
    twistedEnd.end.normal = gp_Dir(0.0, 1.0, 0.0);
    success &= require(!SheetMetalConnectorBuilder::build(twistedEnd).isValid(),
                       "A route with a mismatched end sheet normal was accepted.");

    auto lapWeld = straight;
    lapWeld.start.weld.typeIndex = 1;
    lapWeld.end.weld.typeIndex = 1;
    const auto lapWeldBuild = SheetMetalConnectorBuilder::build(lapWeld);
    success &= require(lapWeldBuild.isValid(),
                       "Lap-weld connector build failed.");
    Bnd_Box lapWeldBox;
    BRepBndLib::Add(lapWeldBuild.solid, lapWeldBox);
    double xMin = 0.0;
    double yMin = 0.0;
    double zMin = 0.0;
    double xMax = 0.0;
    double yMax = 0.0;
    double zMax = 0.0;
    lapWeldBox.Get(xMin, yMin, zMin, xMax, yMax, zMax);
    success &= require(xMin < -9.9,
                       "Lap weld does not extend into the start target.");
    success &= require(xMax > 109.9,
                       "Lap weld does not extend into the end target.");

    const TopoDS_Shape lapTarget = BRepPrimAPI_MakeBox(
        gp_Pnt(-10.0, -10.0, -2.0), 10.0, 20.0, 4.0).Shape();
    lapWeld.start.targetShape = lapTarget;
    std::vector<Handle(AIS_InteractiveObject)> lapTargets;
    lapTargets.push_back(new AIS_Shape(lapTarget));
    const auto allowedLapContact = SheetMetalCollisionChecker::check(
        lapWeldBuild.solid, lapWeld, lapTargets);
    success &= require(allowedLapContact.isAcceptable(),
                       "Lap-weld target overlap was reported as a collision.");

    auto bent = makeData(
        gp_Pnt(0.0, 0.0, 0.0), gp_Dir(1.0, 0.0, 0.0),
        gp_Pnt(80.0, 60.0, 0.0), gp_Dir(0.0, -1.0, 0.0));
    bent.start.jointType = SheetMetalJointType::BoltNut;
    bent.start.boltNut.specIndex = 0;
    success &= require(
        SheetMetalRoutePlanner::computeOrthogonalRoute(bent, 12.0, &errorMessage),
        "Bent route planning failed.");
    success &= require(bent.routePoints.size() == 3,
                       "Bent route does not contain one bend.");

    const auto bentBuild = SheetMetalConnectorBuilder::build(bent);
    success &= require(bentBuild.isValid(), "Bent connector build failed.");
    success &= require(containsSolid(bentBuild.solid),
                       "Bent connector does not contain a solid.");
    success &= require(BRepCheck_Analyzer(bentBuild.solid).IsValid(),
                       "Bent connector is not a valid B-Rep shape.");

    auto sideBendDisabled = makeData(
        gp_Pnt(0.0, 0.0, 0.0), gp_Dir(0.0, 0.0, 1.0),
        gp_Pnt(80.0, 60.0, 0.0), gp_Dir(0.0, 0.0, 1.0));
    sideBendDisabled.routePoints = {
        makeRoutePoint(gp_Pnt(0.0, 0.0, 0.0), 12.0, true),
        makeRoutePoint(gp_Pnt(80.0, 0.0, 0.0), 12.0),
        makeRoutePoint(gp_Pnt(80.0, 60.0, 0.0), 12.0, true)
    };
    sideBendDisabled.allowSideBends = false;
    sideBendDisabled.routePoints[1].transition.type =
        SheetMetalRouteTransitionType::Weld;
    const auto splitSideBendBuild = SheetMetalConnectorBuilder::build(
        sideBendDisabled);
    success &= require(splitSideBendBuild.isValid()
                           && splitSideBendBuild.pieces.size() == 2,
                       "A disabled side bend was not split into two sheets.");
    for (const auto& piece : splitSideBendBuild.pieces) {
        success &= require(BRepCheck_Analyzer(piece).IsValid(),
                           "A welded side-bend sheet is not a valid B-Rep shape.");
    }

    if (splitSideBendBuild.pieces.size() == 2) {
        BRepAlgoAPI_Common common(
            splitSideBendBuild.pieces[0], splitSideBendBuild.pieces[1]);
        common.Build();
        GProp_GProps commonProperties;
        if (common.IsDone() && !common.Shape().IsNull()) {
            BRepGProp::VolumeProperties(common.Shape(), commonProperties);
        }
        success &= require(common.IsDone()
                               && std::abs(commonProperties.Mass())
                                   <= Precision::Confusion(),
                           "Butt-weld corner sheets have an intersecting volume.");
        BRepExtrema_DistShapeShape contactDistance(
            splitSideBendBuild.pieces[0], splitSideBendBuild.pieces[1]);
        contactDistance.Perform();
        success &= require(contactDistance.IsDone()
                               && contactDistance.NbSolution() > 0
                               && contactDistance.Value()
                                   <= Precision::Confusion(),
                           "Butt-weld corner sheets do not have face contact.");
    }

    sideBendDisabled.routePoints[1].transition.type =
        SheetMetalRouteTransitionType::BoltNut;
    success &= require(!SheetMetalConnectorBuilder::build(sideBendDisabled).isValid(),
                       "A bolted corner joint was accepted without an angle bracket.");

    auto uprightBend = makeData(
        gp_Pnt(0.0, 0.0, 0.0), gp_Dir(1.0, 0.0, 0.0),
        gp_Pnt(80.0, 0.0, 60.0), gp_Dir(0.0, 0.0, -1.0));
    uprightBend.allowSideBends = false;
    success &= require(
        SheetMetalRoutePlanner::computeOrthogonalRoute(
            uprightBend, 12.0, &errorMessage),
        "An upright bend was rejected when side bends were disabled.");

    auto twoBends = makeData(
        gp_Pnt(0.0, 0.0, 0.0), gp_Dir(0.0, 0.0, 1.0),
        gp_Pnt(80.0, 60.0, 40.0), gp_Dir(0.0, -1.0, 0.0));
    twoBends.maximumBendCount = 1;
    twoBends.routePoints = {
        makeRoutePoint(gp_Pnt(0.0, 0.0, 0.0), 12.0, true),
        makeRoutePoint(gp_Pnt(80.0, 0.0, 0.0), 12.0),
        makeRoutePoint(gp_Pnt(80.0, 60.0, 0.0), 12.0),
        makeRoutePoint(gp_Pnt(80.0, 60.0, 40.0), 12.0, true)
    };
    twoBends.routePoints[2].transition.type =
        SheetMetalRouteTransitionType::Weld;
    const auto twoSheetBuild = SheetMetalConnectorBuilder::build(twoBends);
    success &= require(twoSheetBuild.isValid()
                           && twoSheetBuild.pieces.size() == 2,
                       "A two-bend route was not split into two sheets.");
    for (const auto& piece : twoSheetBuild.pieces) {
        success &= require(BRepCheck_Analyzer(piece).IsValid(),
                           "A split sheet is not a valid B-Rep shape.");
    }

    auto lapWeldSheets = straight;
    lapWeldSheets.routePoints.insert(
        lapWeldSheets.routePoints.begin() + 1,
        makeRoutePoint(gp_Pnt(50.0, 0.0, 0.0), 12.0));
    lapWeldSheets.routePoints[1].transition.type =
        SheetMetalRouteTransitionType::Weld;
    lapWeldSheets.routePoints[1].transition.weld.typeIndex = 1;
    const auto lapWeldSheetsBuild = SheetMetalConnectorBuilder::build(
        lapWeldSheets);
    success &= require(lapWeldSheetsBuild.isValid()
                           && lapWeldSheetsBuild.pieces.size() == 2,
                       "A lap-weld sheet joint was not generated.");
    if (lapWeldSheetsBuild.pieces.size() == 2) {
        BRepAlgoAPI_Common common(
            lapWeldSheetsBuild.pieces[0], lapWeldSheetsBuild.pieces[1]);
        common.Build();
        GProp_GProps commonProperties;
        if (common.IsDone() && !common.Shape().IsNull()) {
            BRepGProp::VolumeProperties(common.Shape(), commonProperties);
        }
        success &= require(common.IsDone()
                               && std::abs(commonProperties.Mass())
                                   <= Precision::Confusion(),
                           "Lap-weld sheets have an intersecting volume.");

        BRepExtrema_DistShapeShape contactDistance(
            lapWeldSheetsBuild.pieces[0], lapWeldSheetsBuild.pieces[1]);
        contactDistance.Perform();
        success &= require(contactDistance.IsDone()
                               && contactDistance.NbSolution() > 0
                               && contactDistance.Value()
                                   <= Precision::Confusion(),
                           "Lap-weld sheets do not have face contact.");
    }

    auto boltedSheets = lapWeldSheets;
    boltedSheets.routePoints[1].transition.type =
        SheetMetalRouteTransitionType::BoltNut;
    boltedSheets.routePoints[1].transition.boltNut.specIndex = 1;
    const auto boltedSheetsBuild = SheetMetalConnectorBuilder::build(boltedSheets);
    success &= require(boltedSheetsBuild.isValid()
                           && boltedSheetsBuild.pieces.size() == 2,
                       "A straight bolted sheet joint was not generated.");
    for (const auto& piece : boltedSheetsBuild.pieces) {
        success &= require(BRepCheck_Analyzer(piece).IsValid(),
                           "A bolted sheet is not a valid B-Rep shape.");
    }

    auto tooSmallBend = bent;
    tooSmallBend.routePoints[1].transition.bend.radius = 5.0;
    const auto tooSmallBendBuild = SheetMetalConnectorBuilder::build(tooSmallBend);
    success &= require(!tooSmallBendBuild.isValid(),
                       "A self-intersecting bend radius was not rejected.");

    bent.upDirection = gp_Dir(0.0, 1.0, 0.0);
    bent.maximumBendCount = 1;
    bent.allowSideBends = false;
    bent.routePoints[1].transition.type = SheetMetalRouteTransitionType::Weld;
    bent.routePoints[1].transition.weld.typeIndex = 0;
    const QString metadata = SheetMetalConnectorMetadata::serialize(bent);
    SheetMetalConnectorMetadataResult parsedMetadata;
    success &= require(
        SheetMetalConnectorMetadata::deserialize(
            metadata, parsedMetadata, &errorMessage),
        "Connector metadata round trip failed.");
    success &= require(
        parsedMetadata.data.routePoints.size() == bent.routePoints.size()
            && parsedMetadata.data.start.jointType == bent.start.jointType
            && parsedMetadata.data.maximumBendCount == bent.maximumBendCount
            && parsedMetadata.data.allowSideBends == bent.allowSideBends
            && parsedMetadata.data.routePoints[1].transition.type
                == bent.routePoints[1].transition.type
            && parsedMetadata.data.routePoints[1].transition.weld.typeIndex
                == bent.routePoints[1].transition.weld.typeIndex
            && parsedMetadata.data.upDirection.IsEqual(
                bent.upDirection, Precision::Angular()),
        "Connector metadata values were not preserved.");
    success &= require(
        !SheetMetalConnectorMetadata::deserialize(
            QStringLiteral("version=2;width=invalid"),
            parsedMetadata,
            &errorMessage),
        "Malformed connector metadata was accepted.");
    success &= require(
        SheetMetalConnectorMetadata::deserialize(
            QStringLiteral("version=1;width=20;thickness=2;clearance=1;"
                           "start=0,0,0,1,0,0,0,0,0,;"
                           "end=10,0,0,-1,0,0,0,0,0,;"
                           "route=0,0,0,5|10,0,0,5"),
            parsedMetadata,
            &errorMessage),
        "Version 1 connector metadata is not compatible.");
    success &= require(
        SheetMetalConnectorMetadata::deserialize(
            QStringLiteral("version=2;width=20;thickness=2;clearance=1;up=0,0,1;"
                           "start=0,0,0,1,0,0,0,0,0,;"
                           "end=10,0,0,-1,0,0,0,0,0,;"
                           "route=0,0,0,5|10,0,0,5"),
            parsedMetadata,
            &errorMessage),
        "Version 2 connector metadata is not compatible.");
    success &= require(
        parsedMetadata.data.maximumBendCount == 2
            && parsedMetadata.data.allowSideBends,
        "Legacy metadata did not receive manufacturing defaults.");
    success &= require(
        SheetMetalConnectorMetadata::deserialize(
            QStringLiteral("version=3;width=20;thickness=2;clearance=1;up=0,0,1;"
                           "maximumBendCount=1;allowSideBends=0;"
                           "start=0,0,0,1,0,0,0,0,0,;"
                           "end=10,0,0,-1,0,0,0,0,0,;"
                           "route=0,0,0,5|10,0,0,5"),
            parsedMetadata,
            &errorMessage),
        "Version 3 connector metadata is not compatible.");
    success &= require(parsedMetadata.data.routePoints.size() == 2,
                       "Version 3 metadata route was not restored.");

    success &= require(
        SheetMetalConnectorMetadata::deserialize(
            QStringLiteral("version=4;width=20;thickness=2;clearance=1;up=0,0,1;"
                           "maximumBendCount=0;allowSideBends=1;"
                           "intermediateJointType=1;intermediateWeldType=0;"
                           "intermediateBoltSpec=2;"
                           "start=0,0,0,1,0,0,0,0,0,;"
                           "end=10,10,0,0,-1,0,0,0,0,;"
                           "route=0,0,0,12|10,0,0,12|10,10,0,12"),
            parsedMetadata,
            &errorMessage),
        "Version 4 connector metadata is not compatible.");
    success &= require(
        parsedMetadata.data.routePoints.size() == 3
            && parsedMetadata.data.routePoints[1].transition.type
                == SheetMetalRouteTransitionType::BoltNut
            && parsedMetadata.data.routePoints[1].transition.boltNut.specIndex == 2,
        "Version 4 intermediate joint was not migrated to its route node.");

    auto invalidBend = bent;
    invalidBend.routePoints[1].transition.type =
        SheetMetalRouteTransitionType::Bend;
    invalidBend.routePoints[1].transition.bend.radius = 1000.0;
    const auto invalidBuild = SheetMetalConnectorBuilder::build(invalidBend);
    success &= require(!invalidBuild.isValid(),
                       "An oversized bend radius was not rejected.");

    std::vector<Handle(AIS_InteractiveObject)> obstacles;
    const TopoDS_Shape obstacle = BRepPrimAPI_MakeBox(
        gp_Pnt(45.0, -15.0, -10.0), 10.0, 30.0, 20.0).Shape();
    obstacles.push_back(new AIS_Shape(obstacle));
    obstacles.push_back(new AIS_Shape(straightBuild.solid));
    const auto obstacleMap = SheetMetalObstacleMap::build(
        straightBuild.solid, straight, obstacles);
    success &= require(obstacleMap.stats().sceneObjectCount == 2
                           && obstacleMap.stats().excludedObjectCount == 1
                           && obstacleMap.stats().obstacleCount == 1,
                       "Obstacle map statistics are incorrect.");
    const auto collision = SheetMetalCollisionChecker::check(
        straightBuild.solid, straight, obstacleMap);
    success &= require(collision.hasCollision,
                       "An intersecting obstacle was not detected.");

    obstacles.clear();
    const TopoDS_Shape distantObstacle = BRepPrimAPI_MakeBox(
        gp_Pnt(45.0, 100.0, 100.0), 10.0, 10.0, 10.0).Shape();
    obstacles.push_back(new AIS_Shape(distantObstacle));
    const auto clearRoute = SheetMetalCollisionChecker::check(
        straightBuild.solid, straight, obstacles);
    success &= require(clearRoute.isAcceptable(),
                       "A distant obstacle was reported as a collision.");

    return success ? 0 : 1;
}
