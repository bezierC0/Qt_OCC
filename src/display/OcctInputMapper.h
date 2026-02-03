#pragma once

#include <Aspect_VKey.hxx>
#include <QMouseEvent>

namespace OcctInputMapper
{
    //! Map Qt buttons bitmask to virtual keys.
    Aspect_VKeyMouse qtMouseButtons2VKeys(Qt::MouseButtons theButtons);

    //! Map Qt mouse modifiers bitmask to virtual keys.
    Aspect_VKeyFlags qtMouseModifiers2VKeys(Qt::KeyboardModifiers theModifiers);

    //! Map Qt key to virtual key.
    Aspect_VKey qtKey2VKey(int theKey);
}
