// Game frame entry: the engine calls this every server frame,
// it just forwards to the "frame" hook (frameTime in ms, frame number).
// Mods listen with: hook.register("frame", function (frameTime, frameNum) {})
function Game_RunFrame(levelTime, frameNum) {
	hook.run(["frame", levelTime, frameNum]);
}
