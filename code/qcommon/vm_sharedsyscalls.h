// Copyright (C) 2026 Noire's Mod [noire.dev] — GPLv2

void	trap_Print( const char *text );
void	trap_Error( const char *text ) Q_NO_RETURN;
int		trap_Milliseconds( void );
void	trap_Cvar_Register( vmCvar_t *cvar, const char *var_name, const char *value, int flags );
void	trap_Cvar_Update( vmCvar_t *cvar );
void	trap_Cvar_Set( const char *var_name, const char *value );
void    trap_Cvar_Reset( const char *name );
int		trap_Cvar_VariableIntegerValue( const char *var_name );
float	trap_Cvar_VariableValue( const char *var_name );
int	    trap_Cvar_VariableStringBuffer( const char *var_name, char *buffer, int bufsize );
int		trap_Argc( void );
void	trap_Argv( int n, char *buffer, int bufferLength );
void	trap_Args( char *buffer, int bufferLength );
int		trap_FS_FOpenFile( const char *qpath, fileHandle_t *f, fsMode_t mode );
void	trap_FS_Read( void *buffer, int len, fileHandle_t f );
void	trap_FS_Write( const void *buffer, int len, fileHandle_t f );
void	trap_FS_FCloseFile( fileHandle_t f );
int		trap_FS_GetFileList( const char *path, const char *extension, char *listbuf, int bufsize );
int		trap_FS_Seek( fileHandle_t f, long offset, int origin ); // fsOrigin_t
void	trap_SendConsoleCommand( int exec_when, const char *text );
int		trap_RealTime( qtime_t *qtime );