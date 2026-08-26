case TRAP_JS_CONTEXT: VMContext(VMA(1), VMA(2)); return 0;
case TRAP_JS_OPENFILE: return JSOpenFile(VMA(1), qtrue);
case TRAP_JS_LOADSCRIPTS: JSLoadScripts(VMA(1), VMA(2)); return 0;
case TRAP_JS_EVAL: return JSEval(VMA(1), qfalse, qtrue, VMA(2));
case TRAP_JS_CALL: return JSCall(args[1], VMA(2), VMA(3));
