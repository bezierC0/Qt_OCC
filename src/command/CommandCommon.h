#pragma once
#include <QString>
#include <QList>
#include <QMetaType>
#include <gp_Pnt.hxx>

using PointList = QList<gp_Pnt>;
Q_DECLARE_METATYPE(PointList)

namespace CoreApi {

// ---------------------------------------------------------------------------
// Param key constants - shared between Dialog (writer) and Command (reader)
// ---------------------------------------------------------------------------
namespace Param {
    static const QString X       = "x";
    static const QString Y       = "y";
    static const QString Z       = "z";
    static const QString X1      = "x1";
    static const QString Y1      = "y1";
    static const QString Z1      = "z1";
    static const QString X2      = "x2";
    static const QString Y2      = "y2";
    static const QString Z2      = "z2";
    static const QString X3      = "x3";
    static const QString Y3      = "y3";
    static const QString Z3      = "z3";
    static const QString NX      = "nx";
    static const QString NY      = "ny";
    static const QString NZ      = "nz";
    static const QString RADIUS  = "radius";
    static const QString RADIUS1 = "radius1";
    static const QString RADIUS2 = "radius2";
    static const QString MAJOR   = "majorRadius";
    static const QString MINOR   = "minorRadius";
    static const QString WIDTH   = "width";
    static const QString HEIGHT  = "height";
    static const QString DX      = "dx";
    static const QString DY      = "dy";
    static const QString DZ      = "dz";
    static const QString POINTS  = "points";
    static const QString CLOSED  = "closed";
    static const QString DEGREE  = "degree";
} // namespace Param
}
