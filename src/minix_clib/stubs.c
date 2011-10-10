// Newlib stubs implementation

#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include "devman.h"
#include "ioctl.h"
#include "platform.h"
#include "platform_conf.h"
#include "genstd.h"
#include "utils.h"
#include "salloc.h"

#ifdef USE_MULTIPLE_ALLOCATOR
#include "dlmalloc.h"
#else
#include <stdlib.h>
#endif

// Utility function: look in the device manager table and find the index
// for the given name. Returns an index into the device structure, -1 if error.
// Also returns a pointer to the actual file name (without the device part)
static int find_dm_entry( const char* name, char **pactname )
{
  int i;
  const DM_DEVICE* pdev;
  const char* preal;
  char tempname[ DM_MAX_DEV_NAME + 1 ];
  
  // Sanity check for name
  if( name == NULL || *name == '\0' || *name != '/' )
    return -1;
    
  // Find device name
  preal = strchr( name + 1, '/' );
  if( preal == NULL )
  {
    // This shortcut allows to register the "/" filesystem and use it like "/file.ext"
    strcpy( tempname, "/" );
    preal = name;
  }
  else
  {
    if( ( preal - name > DM_MAX_DEV_NAME ) || ( preal - name == 1 ) ) // name too short/too long
      return -1;
    memcpy( tempname, name, preal - name );
    tempname[ preal - name ] = '\0';
  }
    
  // Find device
  for( i = 0; i < dm_get_num_devices(); i ++ )
  {
    pdev = dm_get_device_at( i );
    if( !strcasecmp( tempname, pdev->name ) )
      break;
  }
  if( i == dm_get_num_devices() )
    return -1;
    
  // Find the actual first char of the name
  preal ++;
  if( *preal == '\0' )
    return -1;
  *pactname = ( char * )preal;
  return i;  
}




// *****************************************************************************
// _open_r


int _open_creat( const char *name, int flags, int mode )
{
  char* actname;
  int res, devid;
  const DM_DEVICE* pdev;
 
  // Look for device, return error if not found or if function not implemented
  if( ( devid = find_dm_entry( name, &actname ) ) == -1 )
  {
    return -1; 
  }
  pdev = dm_get_device_at( devid );
  if( pdev->p_open == NULL )
  {
    return ENOSYS;
  }
  
  // Device found, call its function
  if( ( res = pdev->p_open( actname, flags, mode ) ) < 0 )
    return res;
  return DM_MAKE_DESC( devid, res );
}

int _open( const char *name, int flags )
{
  return _open_creat( name, flags, 0);
}

int _creat(const char *name, int mode)
{
  return _open_creat( name, 0, mode);
}

// *****************************************************************************
// _close
int _close( int file )
{
  const DM_DEVICE* pdev;
  
  // Find device, check close function
  pdev = dm_get_device_at( DM_GET_DEVID( file ) );
  if( pdev->p_close == NULL )
  {
    return -1; 
  }
  
  // And call the close function
  return pdev->p_close( DM_GET_FD( file ) );
}

// *****************************************************************************
// _fstat (not implemented)
int _fstat( int file, struct stat *st )
{
  if( ( file >= DM_STDIN_NUM ) && ( file <= DM_STDERR_NUM ) )
  {
    st->st_mode = S_IFCHR;
    return 0;
  }
  return -1;
}

// *****************************************************************************
// _lseek_r
off_t _lseek( int file, off_t off, int whence )
{
  const DM_DEVICE* pdev;
  
  // Find device, check close function
  pdev = dm_get_device_at( DM_GET_DEVID( file ) );
  if( pdev->p_lseek == NULL )
  {
    return -1; 
  }
  
  // And call the close function
  return pdev->p_lseek( DM_GET_FD( file ), off, whence );
}

// *****************************************************************************
// _read_r 
_ssize_t _read( int file, void *ptr, size_t len )
{
  const DM_DEVICE* pdev;
  
  // Find device, check read function
  pdev = dm_get_device_at( DM_GET_DEVID( file ) );
  if( pdev->p_read == NULL )
  {
    return -1; 
  }
  
  // And call the read function
  return pdev->p_read( DM_GET_FD( file ), ptr, len );  
}

// *****************************************************************************
// _write_r 
_ssize_t _write( int file, const void *ptr, size_t len )
{
  const DM_DEVICE* pdev;
  
  // Find device, check write function
  pdev = dm_get_device_at( DM_GET_DEVID( file ) );
  if( pdev->p_write == NULL )
  {
    return -1; 
  }
  
  // And call the write function
  return pdev->p_write( DM_GET_FD( file ), ptr, len );  
}

// ****************************************************************************
// Miscalenous functions

int _isatty( int fd )
{
  return 1;
}

#ifndef WIN32

int isatty( int fd )
{
  return 1;
}
#include <sys/types.h>
#include <unistd.h>

pid_t _getpid()
{
  return 0;
}

// For some very strange reason, the next function is required by the i386 platform...
pid_t getpid()
{
  return 0;
}

#include <sys/times.h>
clock_t _times_r(struct tms *buf )
{
  return 0;
}

int _unlink_r(const char *name )
{
  errno = ENOSYS;
  return -1;
}

int _link_r(const char *c1, const char *c2 )
{
  errno = ENOSYS;
  return -1;
}

#include <sys/time.h>
int _gettimeofday_r( struct timeval *tv, void *tz )
{
  errno = ENOSYS;
  return -1;  
}

#include <stdlib.h>
void _exit( int status )
{
  while( 1 );
}

int _kill( int pid, int sig )
{
  return -1;
}
#endif

// If LUA_NUMBER_INTEGRAL is defined, "redirect" printf/scanf calls to their 
// integer counterparts
#ifdef LUA_NUMBER_INTEGRAL
int _vfprintf_r( FILE *stream, const char *format, va_list ap )
{
  return _vfiprintf_r( stream, format, ap );
}

extern int __svfiscanf_r(FILE *, _CONST char *,va_list);
int __svfscanf_r( FILE *stream, const char *format, va_list ap )
{
  return __svfiscanf_r( stream, format, ap );
}
#endif // #ifdef LUA_NUMBER_INTEGRAL

// ****************************************************************************
// Allocator support

// _sbrk_r (newlib) / elua_sbrk (multiple)
static char *heap_ptr; 
static int mem_index;

#ifdef USE_MULTIPLE_ALLOCATOR
void* elua_sbrk( ptrdiff_t incr )
#else
void* _sbrk( ptrdiff_t incr )
#endif
{
  void* ptr;
      
  // If increment is negative, return -1
  if( incr < 0 )
    return ( void* )-1;
    
  // Otherwise ask the platform about our memory space (if needed)
  // We do this for all our memory spaces
  while( 1 )
  {
    if( heap_ptr == NULL )  
    {
      if( ( heap_ptr = ( char* )platform_get_first_free_ram( mem_index ) ) == NULL )
      {
        ptr = ( void* )-1;
        break;
      }
    }
      
    // Do we have space in the current memory space?
    if( heap_ptr + incr > ( char* )platform_get_last_free_ram( mem_index )  ) 
    {
      // We don't, so try the next memory space
      heap_ptr = NULL;
      mem_index ++;
    }
    else
    {
      // Memory found in the current space
      ptr = heap_ptr;
      heap_ptr += incr;
      break;
    }
  }  

  return ptr;
} 

#if defined( USE_MULTIPLE_ALLOCATOR ) || defined( USE_SIMPLE_ALLOCATOR )
// Redirect all allocator calls to our dlmalloc/salloc 

#ifdef USE_MULTIPLE_ALLOCATOR
#define CNAME( func ) dl##func
#else
#define CNAME( func ) s##func
#endif

void* _malloc_r( size_t size )
{
  return CNAME( malloc )( size );
}

void* _calloc_r( size_t nelem, size_t elem_size )
{
  return CNAME( calloc )( nelem, elem_size );
}

void _free_r( void* ptr )
{
  CNAME( free )( ptr );
}

void* _realloc_r( void* ptr, size_t size )
{
  return CNAME( realloc )( ptr, size );
}

#endif // #ifdef USE_MULTIPLE_ALLOCATOR

// *****************************************************************************
// eLua stubs (not Newlib specific)

#if !defined( BUILD_CON_GENERIC ) && !defined( BUILD_CON_TCP )

// Set send/recv functions
void std_set_send_func( p_std_send_char pfunc )
{
}

void std_set_get_func( p_std_get_char pfunc )
{
}

const DM_DEVICE* std_get_desc()
{
  return NULL;
}

#endif // #if !defined( BUILD_CON_GENERIC ) && !defined( BUILD_CON_TCP )

// ****************************************************************************
// memcpy is broken on AVR32's Newlib, so impolement a simple version here
// same goes for strcmp apparently
#ifdef FORAVR32
void* memcpy( void *dst, const void* src, size_t len )
{
  char *pdest = ( char* )dst;
  const char* psrc = ( const char* )src;
  
  while( len )
  {
    *pdest ++ = *psrc ++;
    len --;
  }
  return dst;
}

int strcmp(const char *s1, const char *s2)
{
  while( *s1 == *s2++ ) 
  {
    if( *s1++ == '\0' )
      return 0;
  }
  if( *s1 == '\0' ) 
    return -1;
  if( *--s2 == '\0' ) 
    return 1;
  return ( unsigned char )*s1 - ( unsigned char )*s2;
}

#endif // #ifdef FORAVR32
