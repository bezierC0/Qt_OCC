#pragma once
#include <QString>

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
    static const QString NX      = "nx";
    static const QString NY      = "ny";
    static const QString NZ      = "nz";
    static const QString RADIUS  = "radius";
    static const QString MAJOR   = "majorRadius";
    static const QString MINOR   = "minorRadius";
    static const QString WIDTH   = "width";
    static const QString HEIGHT  = "height";
} // namespace Param
}