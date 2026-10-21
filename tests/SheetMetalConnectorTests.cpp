#include "SheetMetalCollisionChecker.h"
#include "SheetMetalConnectorBuilder.h"
#include "SheetMetalFaceGeometry.h"
#include "SheetMetalConnectorMetadata.h"
#include "SheetMetalRoutePlanner.h"

#include <AIS_Shape.hxx>
#include <BRepPrimAPI_MakeBox.hxx>
#include <BRep_Tool.hxx>
#include <BRepClass_FaceClassifier.hxx>
#include <Precision.hxx>
#include <TopAbs_ShapeEnum.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopLoc_Location.hxx>

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
    return data;
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
        gp_Pnt(0.0, 0.0, 0.0), gp_Dir(1.0, 0.0, 0.0),
        gp_Pnt(100.0, 0.0, 0.0), gp_Dir(-1.0, 0.0, 0.0));
    QString errorMessage;
    success &= require(
        SheetMetalRoutePlanner::computeOrthogonalRoute(straight, 5.0, &errorMessage),
        "Straight route planning failed.");
    success &= require(straight.routePoints.size() == 2,
                       "Straight route contains unexpected waypoints.");

    const auto straightBuild = SheetMetalConnectorBuilder::build(straight);
    success &= require(straightBuild.isValid(), "Straight connector build failed.");
    success &= require(containsSolid(straightBuild.solid),
                       "Straight connector does not contain a solid.");

    auto bent = makeData(
        gp_Pnt(0.0, 0.0, 0.0), gp_Dir(1.0, 0.0, 0.0),
        gp_Pnt(80.0, 60.0, 0.0), gp_Dir(0.0, -1.0, 0.0));
    bent.start.jointType = SheetMetalJointType::BoltNut;
    bent.start.boltNut.specIndex = 0;
    success &= require(
        SheetMetalRoutePlanner::computeOrthogonalRoute(bent, 8.0, &errorMessage),
        "Bent route planning failed.");
    success &= require(bent.routePoints.size() == 3,
                       "Bent route does not contain one bend.");

    const auto bentBuild = SheetMetalConnectorBuilder::build(bent);
    success &= require(bentBuild.isValid(), "Bent connector build failed.");
    success &= require(containsSolid(bentBuild.solid),
                       "Bent connector does not contain a solid.");

    bent.upDirection = gp_Dir(0.0, 1.0, 0.0);
    const QString metadata = SheetMetalConnectorMetadata::serialize(bent);
    SheetMetalConnectorMetadataResult parsedMetadata;
    success &= require(
        SheetMetalConnectorMetadata::deserialize(
            metadata, parsedMetadata, &errorMessage),
        "Connector metadata round trip failed.");
    success &= require(
        parsedMetadata.data.routePoints.size() == bent.routePoints.size()
            && parsedMetadata.data.start.jointType == bent.start.jointType
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

    auto invalidBend = bent;
    invalidBend.routePoints[1].bend.radius = 1000.0;
    const auto invalidBuild = SheetMetalConnectorBuilder::build(invalidBend);
    success &= require(!invalidBuild.isValid(),
                       "An oversized bend radius was not rejected.");

    std::vector<Handle(AIS_InteractiveObject)> obstacles;
    const TopoDS_Shape obstacle = BRepPrimAPI_MakeBox(
        gp_Pnt(45.0, -15.0, -10.0), 10.0, 30.0, 20.0).Shape();
    obstacles.push_back(new AIS_Shape(obstacle));
    const auto collision = SheetMetalCollisionChecker::check(
        straightBuild.solid, straight, obstacles);
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
