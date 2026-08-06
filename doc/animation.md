# Animation

The Animation tool creates and previews simple presentation sequences for
objects displayed in an OpenCASCADE view. It supports object transforms, scene
visibility, camera movement, timeline editing, and MP4 export.

## Supported actions

| Action | Target | Settings |
| --- | --- | --- |
| Translation | Picked object | Direction, distance, rate, easing |
| Rotation | Picked object | Global axis, angle, rate, easing |
| Visibility | Picked object | Show or hide, duration |
| Camera | Active OCC view | Captured start/end cameras, duration, easing |

Actions can run **After Previous** or **With Previous (Parallel)**. Parallel
actions may target different objects, but two actions in the same parallel
group cannot target the same object. A parallel group ends when its longest
action ends.

## Basic workflow

1. Open the Animation tool from the application.
2. Select an action type.
3. For an object action, click **Pick** and select the target in the OCC view.
   The selected target name is shown beside **Target**. Click **Cancel** to
   leave picking mode without changing the current target.
4. Configure the action values and timing.
5. Click **Add**.
6. Repeat for the remaining actions, then use **Play**, **Pause**, **Rewind**,
   or **Loop** to inspect the sequence.

For a Camera action, set the view to its first pose and click **Capture Camera
Start**. Set the final pose and click **Capture Camera End**, then add the
action.

## Editing and preview

- Select an action and click **Update** to replace its settings.
- Use **Move Up** and **Move Down** to change execution order.
- Use **Duplicate** to create a copy of the selected action.
- Use **Preview Step** to inspect one action.
- Drag the timeline slider to evaluate the complete sequence at an exact time.

Transform actions on the same object are cumulative: each action starts from
the result of that object's previous action. Closing or hiding the non-modal
Animation window stops playback, restores the original object and camera state,
and requests a redraw of the originating OCC view.

## Video export

Set **Video FPS** and click **Export Video...** to create an MP4 file. Export is
evaluated offline at a fixed frame rate, so it is independent of interactive
playback speed. Progress can be cancelled, and the original scene is restored
after success, cancellation, or failure.

Video encoding uses OpenCASCADE `Image_VideoRecorder`. This OCCT API uses the
FFmpeg libraries supplied as OCCT third-party runtime dependencies; a separate
`ffmpeg.exe` on `PATH` is not required.

The output duration is quantized to whole frames. The maximum rounding error is
approximately half a frame (`0.5 / FPS` seconds). The captured viewport is
cropped by at most one pixel in each dimension when an even size is required by
the MP4 pixel format.

## Current limitations

- Animation sequences are not persisted yet. STEP re-import does not provide a
  stable application-level object ID, so a separately saved sequence cannot
  safely reconnect every action to its original target.
- Actions use global X, Y, and Z directions or axes.
- Video resolution is derived from the current OCC viewport.
- Automated tests cover the sequence model. Picking, OpenGL rendering, camera
  playback, window lifecycle, and encoded video content still require
  interactive application testing.

## Implementation overview

- `AnimationSequence` owns action validation, scheduling, cumulative transform
  composition, easing data, and timeline segments.
- `WidgetAnimation` owns the tool UI, picking, playback, scene tracks, preview,
  restoration, and video export.
- `ViewerWidget` only opens and owns the non-modal tool and handles its queued
  redraw request.
- `OCCView` provides the active OCCT view and lightweight redraw requests.

See [Animation Optimization Plan](animation_optimization_plan.md) for the
development history and remaining optional work.
