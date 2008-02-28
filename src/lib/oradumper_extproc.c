#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>

#if HAVE_ASSERT_H
#include <assert.h>
#endif

#if HAVE_STDLIB_H
#include <stdlib.h>
#endif

#if HAVE_STRING_H
#include <string.h>
#endif

#ifdef SQLCA_NONE 
#undef SQLCA_NONE 
#endif
#define SQLCA_NONE 

#ifdef SQLCA_STORAGE_CLASS
#undef SQLCA_STORAGE_CLASS
#endif

#if HAVE_SQLCPR_H
#define ORACA 1
#include <sqlcpr.h>
#endif

#include <oci.h>

/* include dmalloc as last one */
#ifdef WITH_DMALLOC
#include <dmalloc.h>
#endif

#include "oradumper.h"
#include "oradumper_int.h"

void oradumper_extproc(OCIExtProcContext *ctx, OCIColl *coll, short coll_ind)
{
  const int disconnect = 0;
  char error_msg[1000+1];
  OCIEnv *envhp;
  OCISvcCtx *svchp;
  OCIError *errhp;
  unsigned int length;
  char *msg;

  if (orasql_register_connect(ctx) != OK)
    {
      orasql_error(&length, &msg);
      SQLExtProcError(SQL_SINGLE_RCTX, msg, (size_t)length);
    }
  else if ( (SQLEnvGet(0, &envhp) != OCI_SUCCESS) ||
	    (OCIHandleAlloc(envhp, (dvoid**)&errhp, OCI_HTYPE_ERROR, 0, 0) != OCI_SUCCESS) ||
	    (SQLSvcCtxGet(0, NULL, 0, &svchp) != OCI_SUCCESS) )
    {
      msg = "failed to setup OCI environment";
      SQLExtProcError(SQL_SINGLE_RCTX, msg, strlen(msg));
    }
  else
    {
      sb4 array_size;
      boolean exists;
      OCIString **ocistring;
      sb4 i;
      oratext *txt;
      char **arguments = NULL;

      if ( coll_ind != OCI_IND_NULL &&
	   OCICollSize(envhp, errhp, coll, &array_size) == OCI_SUCCESS )
	{
	  arguments = (char **) calloc(array_size, sizeof(*arguments));
	  assert(arguments != NULL);

	  for (i = 0; i < array_size; i++)
	    {
	      if (OCICollGetElem(envhp, errhp, coll, i, &exists,
				 (dvoid*)&ocistring, 0) == OCI_SUCCESS &&
		  exists)
		{
		  txt = OCIStringPtr(envhp, *ocistring);
		  arguments[i] = (char *) malloc(strlen((char *)txt)+1);
		  assert(arguments[i] != NULL);
		  (void) strcpy(arguments[i], (char *)txt);
		}
	      else
		arguments[i] = NULL;
	    }
	}

      if (NULL != (msg = oradumper((unsigned int)array_size,
				   (const char **)arguments,
				   disconnect,
				   sizeof(error_msg),
				   error_msg)))
	{
	  SQLExtProcError(SQL_SINGLE_RCTX, msg, strlen(msg));
	}
      else
	{
	  for (i = 0; i < array_size; i++)
	    {
#ifndef lint
	      if (arguments[i] != NULL)
#endif
		{
		  free(arguments[i]);
		}
	    }
#ifndef lint
	  if (arguments != NULL)
#endif
	    free(arguments);
	}
    }
}
