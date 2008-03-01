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

#if HAVE_DBUG_H
#include <dbug.h>
#endif

/* include dmalloc as last one */
#ifdef WITH_DMALLOC
#include <dmalloc.h>
#endif

#include <sql2oci.h>

#include "oradumper.h"
#include "oradumper_int.h"

typedef /*@null@*/ /*@only@*/ char *char_ptr_t;

void oradumper_extproc(OCIExtProcContext *ctx, OCIColl *coll, short coll_ind)
{
  const int disconnect = 0;
  char error_msg[1000+1];
  OCIEnv *envhp = NULL;
  OCISvcCtx *svchp = NULL;
  OCIError *errhp = NULL;
  unsigned int length = 0;
  /*@observer@*/ char *msg = NULL;

  DBUG_INIT(getenv("DBUG_OPTIONS"), "oradumper_extproc");
  DBUG_ENTER("oradumper_extproc");

  if (orasql_register_connect(ctx) != OK)
    {
      orasql_error(&length, &msg);
    }
  else if (/*@-nullpass@*/
	   SQLEnvGet(0, &envhp) != OCI_SUCCESS ||
	   OCIHandleAlloc(envhp, (void **)&errhp, OCI_HTYPE_ERROR, 0, 0) != OCI_SUCCESS ||
	   SQLSvcCtxGet(0, NULL, 0, &svchp) != OCI_SUCCESS 
	   /*@=nullpass@*/)
    {
      msg = "failed to setup OCI environment";
    }
  else
    {
      sb4 array_size = 0;
      boolean exists = false;
      OCIString **ocistring = NULL;
      sb4 i, j; /* j is the Nth non-NULL entry */
      oratext *txt = NULL;
      /*@null@ @only@*/ char_ptr_t *arguments = NULL;

      /*@-nullpass@*/
      DBUG_PRINT("info", ("envhp: %p; errhp: %p; svchp: %p", envhp, errhp, svchp));
      /*@=nullpass@*/

      /* just assume */
      assert(envhp != NULL);

      /* OCIHandleAlloc returns a non-NULL handle on success */
      assert(errhp != NULL);

      if ( coll_ind != OCI_IND_NULL &&
	   OCICollSize(envhp, errhp, coll, &array_size) == OCI_SUCCESS )
	{
	  DBUG_PRINT("info", ("array_size: %d", (int) array_size));

	  arguments = (char **) calloc((size_t) array_size, sizeof(*arguments));
	  assert(arguments != NULL);

	  /* Make sure arguments starts with non-NULL entries only. */

	  for (i = 0; i < array_size; i++)
	    arguments[i] = NULL;

	  for (i = j = 0; i < array_size; i++)
	    {
	      if (/*@-nullpass@*/
		  OCICollGetElem(envhp, errhp, coll, i, &exists,
				 (void **) &ocistring, 0) == OCI_SUCCESS
		  /*@=nullpass@*/ &&
		  exists != 0)
		{
		  assert(ocistring != NULL);
		  txt = OCIStringPtr(envhp, *ocistring);
		  DBUG_PRINT("info", ("array[%d]: '%s'", i, (char*) txt));

		  if (txt[0] != (unsigned char) '\0')
		    {
		      arguments[j++] = strdup((char *)txt);
		    }
		}
	      else
		{
		  DBUG_PRINT("info", ("array[%d]: NULL", i));
		}
	    }

	  msg = oradumper((unsigned int) j,
			  (const char **) arguments,
			  disconnect,
			  sizeof(error_msg),
			  error_msg);
	}
      else
	{
	  DBUG_PRINT("warning", ("Could not determine array size"));
	}

      if (arguments != NULL)
	{
	  for (i = 0; i < array_size; i++)
	    {
	      if (arguments[i] != NULL)
		{
		  free(arguments[i]);
		}
	    }
	  free(arguments);
	}

      if (errhp != NULL)
	{
	  (void) OCIHandleFree(errhp, OCI_HTYPE_ERROR);
	  errhp = NULL;
	}
    }

  /* free handle */
  if (errhp != NULL)
    {
      (void) OCIHandleFree(errhp, OCI_HTYPE_ERROR);
    }

  if (msg != NULL)
    {
      /* set length if not already set */
      if (length == 0)
	{
	  length = (unsigned int) strlen(msg);
	}

      DBUG_PRINT("error", ("msg: %*.*s", length, length, msg));

      /*@-nullpass@*/
      (void) OCIExtProcRaiseExcpWithMsg(ctx, 20000, (unsigned char*) msg, (size_t)length);
      /*@=nullpass@*/
    }

  DBUG_LEAVE();
  DBUG_DONE();
}
