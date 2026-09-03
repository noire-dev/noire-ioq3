#define MAX_JS_ARGS 16
#define MAX_JS_STRINGSIZE 256

typedef enum { JS_TYPE_NONE, JS_TYPE_INT, JS_TYPE_FLOAT, JS_TYPE_STRING } js_type_t;

typedef struct {
	int i;
	float f;
	char s[MAX_JS_STRINGSIZE];
} js_value_t;

typedef struct {
	js_type_t t[MAX_JS_ARGS];
	js_value_t v[MAX_JS_ARGS];
} js_args_t;

typedef struct {
	js_type_t t;
	js_value_t v;
} js_result_t;

void trap_Print( const char *string ) {
	syscall( TRAP_PRINT, string );
}

void trap_Error(const char *string) {
	syscall(TRAP_ERROR, string);
	exit(1); // shut up GCC warning about returning functions, because we know better
}

int trap_Milliseconds( void ) {
	return syscall( TRAP_MILLISECONDS ); 
}

void trap_Cvar_Register( vmCvar_t *cvar, const char *var_name, const char *value, int flags ) {
	syscall( TRAP_CVAR_REGISTER, cvar, var_name, value, flags );
}

void trap_Cvar_Update( vmCvar_t *cvar ) {
	syscall( TRAP_CVAR_UPDATE, cvar );
}

void trap_Cvar_Set( const char *var_name, const char *value ) {
	syscall( TRAP_CVAR_SET, var_name, value );
}

void trap_Cvar_Reset( const char *name ) {
	syscall( TRAP_CVAR_RESET, name ); 
}

int trap_Cvar_VariableIntegerValue( const char *var_name ) {
	return syscall( TRAP_CVAR_VARIABLEINTEGERVALUE, var_name );
}

float trap_Cvar_VariableValue( const char *var_name ) {
	floatint_t fi;
	fi.i = syscall( TRAP_CVAR_VARIABLEVALUE, var_name );
	return fi.f;
}

int trap_Cvar_VariableStringBuffer( const char *var_name, char *buffer, int bufsize ) {
	return syscall( TRAP_CVAR_VARIABLESTRINGBUFFER, var_name, buffer, bufsize );
}

int trap_Argc( void ) {
	return syscall( TRAP_ARGC );
}

void trap_Argv( int n, char *buffer, int bufferLength ) {
	syscall( TRAP_ARGV, n, buffer, bufferLength );
}

void trap_Args( char *buffer, int bufferLength ) {
	syscall( TRAP_ARGS, buffer, bufferLength );
}

int trap_FS_FOpenFile( const char *qpath, fileHandle_t *f, fsMode_t mode ) {
	return syscall( TRAP_FS_FOPENFILE, qpath, f, mode );
}

void trap_FS_Read( void *buffer, int len, fileHandle_t f ) {
	syscall( TRAP_FS_READ, buffer, len, f );
}

void trap_FS_Write( const void *buffer, int len, fileHandle_t f ) {
	syscall( TRAP_FS_WRITE, buffer, len, f );
}

void trap_FS_FCloseFile( fileHandle_t f ) {
	syscall( TRAP_FS_FCLOSEFILE, f );
}

int trap_FS_GetFileList(  const char *path, const char *extension, char *listbuf, int bufsize ) {
	return syscall( TRAP_FS_GETFILELIST, path, extension, listbuf, bufsize );
}

int trap_FS_Seek( fileHandle_t f, long offset, int origin ) {
	return syscall( TRAP_FS_SEEK, f, offset, origin );
}

void trap_SendConsoleCommand( int exec_when, const char *text ) {
	syscall( TRAP_SEND_CONSOLE_COMMAND, exec_when, text );
}

int trap_RealTime(qtime_t *qtime) {
	return syscall( TRAP_REAL_TIME, qtime );
}

void VMContext(js_args_t* args, js_result_t* result) {
	syscall( TRAP_JS_CONTEXT, args, result );
}

bool JSOpenFile(const char* filename) {
	return syscall( TRAP_JS_OPENFILE, filename );
}

void JSLoadScripts(const char* path, const char* name) {
	syscall( TRAP_JS_LOADSCRIPTS, path, name );
}

bool JSEval(const char* code, js_result_t* result) {
	return syscall( TRAP_JS_EVAL, code, result );
}

bool JSCall(int id, js_args_t* args, js_result_t* result) {
	return syscall( TRAP_JS_CALL, id, args, result );
}
