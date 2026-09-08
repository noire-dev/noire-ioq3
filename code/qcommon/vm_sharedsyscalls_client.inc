void trap_UpdateScreen(void) { syscall(TRAP_UPDATESCREEN); }
void trap_GetGlconfig(glconfig_t* glconfig) { syscall(TRAP_GETGLCONFIG, glconfig); }
qhandle_t trap_R_RegisterModel(const char* name) { return syscall(TRAP_R_REGISTERMODEL, name); }
qhandle_t trap_R_RegisterSkin(const char* name) { return syscall(TRAP_R_REGISTERSKIN, name); }
sfxHandle_t trap_S_RegisterSound(const char* sample, bool compressed) { return syscall(TRAP_S_REGISTERSOUND, sample, compressed); }
qhandle_t trap_R_RegisterShader(const char* name) { return syscall(TRAP_R_REGISTERSHADER, name); }
qhandle_t trap_R_RegisterShaderNoMip(const char* name) { return syscall(TRAP_R_REGISTERSHADERNOMIP, name); }
void trap_R_ClearScene(void) { syscall(TRAP_R_CLEARSCENE); }
void trap_R_AddRefEntityToScene(const refEntity_t* re) { syscall(TRAP_R_ADDREFENTITYTOSCENE, re); }
void trap_R_AddPolyToScene(qhandle_t hShader, int numVerts, const polyVert_t* verts) { syscall(TRAP_R_ADDPOLYTOSCENE, hShader, numVerts, verts); }
void trap_R_AddLightToScene(const vec3_t org, float intensity, float r, float g, float b) { syscall(TRAP_R_ADDLIGHTTOSCENE, org, PASSFLOAT(intensity), PASSFLOAT(r), PASSFLOAT(g), PASSFLOAT(b)); }
void trap_R_RenderScene(const refdef_t* fd) { syscall(TRAP_R_RENDERSCENE, fd); }
void trap_R_SetColor(const float* rgba) { syscall(TRAP_R_SETCOLOR, rgba); }
void trap_R_DrawStretchPic(float x, float y, float w, float h, float s1, float t1, float s2, float t2, qhandle_t hShader) { syscall(TRAP_R_DRAWSTRETCHPIC, PASSFLOAT(x), PASSFLOAT(y), PASSFLOAT(w), PASSFLOAT(h), PASSFLOAT(s1), PASSFLOAT(t1), PASSFLOAT(s2), PASSFLOAT(t2), hShader); }
void trap_R_RemapShader(const char* oldShader, const char* newShader, const char* timeOffset) { syscall(TRAP_R_REMAP_SHADER, oldShader, newShader, timeOffset); }
void trap_R_ModelBounds(clipHandle_t model, vec3_t mins, vec3_t maxs) { syscall(TRAP_R_MODELBOUNDS, model, mins, maxs); }
int trap_CM_LerpTag(orientation_t* tag, clipHandle_t mod, int startFrame, int endFrame, float frac, const char* tagName) { return syscall(TRAP_CM_LERPTAG, tag, mod, startFrame, endFrame, PASSFLOAT(frac), tagName); }
int trap_CM_PointContents(const vec3_t p, clipHandle_t model) { return syscall(TRAP_CM_POINTCONTENTS, p, model); }
int trap_CM_TransformedPointContents(const vec3_t p, clipHandle_t model, const vec3_t origin, const vec3_t angles) { return syscall(TRAP_CM_TRANSFORMEDPOINTCONTENTS, p, model, origin, angles); }
void trap_CM_BoxTrace(trace_t* results, const vec3_t start, const vec3_t end, const vec3_t mins, const vec3_t maxs, clipHandle_t model, int brushmask) { syscall(TRAP_CM_BOXTRACE, results, start, end, mins, maxs, model, brushmask); }
void trap_CM_CapsuleTrace(trace_t* results, const vec3_t start, const vec3_t end, const vec3_t mins, const vec3_t maxs, clipHandle_t model, int brushmask) { syscall(TRAP_CM_CAPSULETRACE, results, start, end, mins, maxs, model, brushmask); }
void trap_CM_TransformedBoxTrace(trace_t* results, const vec3_t start, const vec3_t end, const vec3_t mins, const vec3_t maxs, clipHandle_t model, int brushmask, const vec3_t origin, const vec3_t angles) { syscall(TRAP_CM_TRANSFORMEDBOXTRACE, results, start, end, mins, maxs, model, brushmask, origin, angles); }
void trap_CM_TransformedCapsuleTrace(trace_t* results, const vec3_t start, const vec3_t end, const vec3_t mins, const vec3_t maxs, clipHandle_t model, int brushmask, const vec3_t origin, const vec3_t angles) { syscall(TRAP_CM_TRANSFORMEDCAPSULETRACE, results, start, end, mins, maxs, model, brushmask, origin, angles); }
void trap_S_StartLocalSound(sfxHandle_t sfx, int channelNum) { syscall(TRAP_S_STARTLOCALSOUND, sfx, channelNum); }
void trap_Key_KeynumToStringBuf(int keynum, char* buf, int buflen) { syscall(TRAP_KEY_KEYNUMTOSTRINGBUF, keynum, buf, buflen); }
void trap_Key_GetBindingBuf(int keynum, char* buf, int buflen) { syscall(TRAP_KEY_GETBINDINGBUF, keynum, buf, buflen); }
bool trap_Key_IsDown(int keynum) { return syscall(TRAP_KEY_ISDOWN, keynum); }
int trap_Key_GetCatcher(void) { return syscall(TRAP_KEY_GETCATCHER); }
int trap_Key_GetKey(const char* binding) { return syscall(TRAP_KEY_GETKEY, binding); }
void trap_ImportOBJ(const char* filename) { syscall(TRAP_IMPORTOBJ, filename); }
