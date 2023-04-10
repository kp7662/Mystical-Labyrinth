/*--------------------------------------------------------------------*/
/* ft.c                                                               */
/* Author: Kok Wei Pua and Cherie Jiraphanphong                                        */
/*--------------------------------------------------------------------*/

#include <stddef.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "dynarray.h"
#include "path.h"
#include "node.h"
#include "ft.h"

/*
  A Directory-File Tree is a representation of a hierarchy of directories and files,
  represented as an AO with 3 state variables:
*/

/* 1. a flag for being in an initialized state (TRUE) or not (FALSE) */
static boolean bIsInitialized;
/* 2. a pointer to the root node in the hierarchy */
static Node_T oNRoot;
/* 3. a counter of the number of nodes in the hierarchy */
static size_t ulCount;

static int DT_traversePath(Path_T oPPath, Node_T *poNFurthest) {
   int iStatus;
   Path_T oPPrefix = NULL;
   Node_T oNCurr;
   Node_T oNChild = NULL;
   size_t ulDepth;
   size_t i;
   size_t ulChildID;

   assert(oPPath != NULL);
   assert(poNFurthest != NULL);

   /* root is NULL -> won't find anything */
   if(oNRoot == NULL) {
      *poNFurthest = NULL;
      return SUCCESS;
   }

   iStatus = Path_prefix(oPPath, 1, &oPPrefix);
   if(iStatus != SUCCESS) {
      *poNFurthest = NULL;
      return iStatus;
   }

   if(Path_comparePath(Node_getPath(oNRoot), oPPrefix)) {
      Path_free(oPPrefix);
      *poNFurthest = NULL;
      return CONFLICTING_PATH;
   }
   Path_free(oPPrefix);
   oPPrefix = NULL;

   oNCurr = oNRoot;
   ulDepth = Path_getDepth(oPPath);
   for(i = 2; i <= ulDepth; i++) {
      iStatus = Path_prefix(oPPath, i, &oPPrefix);
      if(iStatus != SUCCESS) {
         *poNFurthest = NULL;
         return iStatus;
      }
      if(Node_hasChild(oNCurr, oPPrefix, &ulChildID)) {
         /* go to that child and continue with next prefix */
         Path_free(oPPrefix);
         oPPrefix = NULL;
         iStatus = Node_getChild(oNCurr, ulChildID, &oNChild);
         if(iStatus != SUCCESS) {
            *poNFurthest = NULL;
            return iStatus;
         }
         oNCurr = oNChild;
      }
      else {
         break;
      }
   }

   Path_free(oPPrefix);
   *poNFurthest = oNCurr;
   return SUCCESS;
}

static int DT_findNode(const char *pcPath, Node_T *poNResult) {
   Path_T oPPath = NULL;
   Node_T oNFound = NULL;
   int iStatus;

   assert(pcPath != NULL);
   assert(poNResult != NULL);

   if(!bIsInitialized) {
      *poNResult = NULL;
      return INITIALIZATION_ERROR;
   }

   iStatus = Path_new(pcPath, &oPPath);
   if(iStatus != SUCCESS) {
      *poNResult = NULL;
      return iStatus;
   }

   iStatus = DT_traversePath(oPPath, &oNFound);
   if(iStatus != SUCCESS)
   {
      Path_free(oPPath);
      *poNResult = NULL;
      return iStatus;
   }

   if(oNFound == NULL) {
      Path_free(oPPath);
      *poNResult = NULL;
      return NO_SUCH_PATH;
   }

   if(Path_comparePath(Node_getPath(oNFound), oPPath) != 0) {
      Path_free(oPPath);
      *poNResult = NULL;
      return NO_SUCH_PATH;
   }

   Path_free(oPPath);
   *poNResult = oNFound;
   return SUCCESS;
}

int FT_insertDir(const char *pcPath) {
   int iStatus;
   Path_T oPPath = NULL;
   Node_T oNFirstNew = NULL;
   Node_T oNCurr = NULL;
   size_t ulDepth, ulIndex;
   size_t ulNewNodes = 0;

   assert(pcPath != NULL);
   /* assert(CheckerDT_isValid(bIsInitialized, oNRoot, ulCount)); */

   /* validate pcPath and generate a Path_T for it */
   if(!bIsInitialized)
      return INITIALIZATION_ERROR;

   iStatus = Path_new(pcPath, &oPPath);
   if(iStatus != SUCCESS)
      return iStatus;

   /* find the closest ancestor of oPPath already in the tree */
   iStatus= FT_traversePath(oPPath, &oNCurr);
   if(iStatus != SUCCESS)
   {
      Path_free(oPPath);
      return iStatus;
   }

   /* Check whether if parent is a file */
   if (oNCurr == NULL && oNCurr->isFile == TRUE) {
        return NOT_A_DIRECTORY;
     }

   /* no ancestor node found, so if root is not NULL,
      pcPath isn't underneath root. */
   if(oNCurr == NULL && oNRoot != NULL) {
      Path_free(oPPath);
      return CONFLICTING_PATH;
   }

   ulDepth = Path_getDepth(oPPath);
   if(oNCurr == NULL) /* new root! */
      ulIndex = 1;
   else {
      ulIndex = Path_getDepth(Node_getPath(oNCurr))+1;

      /* oNCurr is the node we're trying to insert */
      if(ulIndex == ulDepth+1 && !Path_comparePath(oPPath,
                                       Node_getPath(oNCurr))) {
         Path_free(oPPath);
         return ALREADY_IN_TREE;
      }
   }

   /* starting at oNCurr, build rest of the path one level at a time */
   while(ulIndex <= ulDepth) {
      Path_T oPPrefix = NULL;
      Node_T oNNewNode = NULL;

      /* generate a Path_T for this level */
      iStatus = Path_prefix(oPPath, ulIndex, &oPPrefix);
      if(iStatus != SUCCESS) {
         Path_free(oPPath);
         if(oNFirstNew != NULL)
            (void) Node_free(oNFirstNew);
         /* assert(CheckerDT_isValid(bIsInitialized, oNRoot, ulCount)); */
         return iStatus;
      }

      /* insert the new node for this level */
      iStatus = Node_new(oPPrefix, oNCurr, &oNNewNode, FALSE, NULL, 0);
      if(iStatus != SUCCESS) {
         Path_free(oPPath);
         Path_free(oPPrefix);
         if(oNFirstNew != NULL)
            (void) Node_free(oNFirstNew);
         /* assert(CheckerDT_isValid(bIsInitialized, oNRoot, ulCount)); */
         return iStatus;
      }

      /* set up for next level */
      Path_free(oPPrefix);
      oNCurr = oNNewNode;
      ulNewNodes++;
      if(oNFirstNew == NULL)
         oNFirstNew = oNCurr;
      ulIndex++;
   }

   Path_free(oPPath);
   /* update FT state variables to reflect insertion */
   if(oNRoot == NULL)
      oNRoot = oNFirstNew;
   ulCount += ulNewNodes;

   /* assert(CheckerDT_isValid(bIsInitialized, oNRoot, ulCount)); */
   return SUCCESS;
}


boolean FT_containsDir(const char *pcPath) {
   int iStatus;
   Node_T oNFound = NULL;

   assert(pcPath != NULL);

   iStatus = FT_findNode(pcPath, &oNFound);

   if(oNFound->isFile == TRUE) {
    iStatus = NOT_A_DIRECTORY; 
   } 

   return (boolean) (iStatus == SUCCESS);
}

int FT_rmDir(const char *pcPath) {
   int iStatus;
   Node_T oNFound = NULL;

   assert(pcPath != NULL);
   /* assert(CheckerDT_isValid(bIsInitialized, oNRoot, ulCount)); */

   iStatus = DT_findNode(pcPath, &oNFound);

   if(oNFound->isFile == TRUE) {
    return NOT_A_DIRECTORY;
   }

   if(iStatus != SUCCESS)
       return iStatus;

   ulCount -= Node_free(oNFound);
   if(ulCount == 0)
      oNRoot = NULL;

   /* assert(CheckerDT_isValid(bIsInitialized, oNRoot, ulCount)); */ 
   return SUCCESS;
}

int FT_insertFile(const char *pcPath, void *pvContents,
                  size_t ulLength) {
   int iStatus;
   Path_T oPPath = NULL;
   Node_T oNFirstNew = NULL;
   Node_T oNCurr = NULL;
   size_t ulDepth, ulIndex;
   size_t ulNewNodes = 0;

   assert(pcPath != NULL);
   /* assert(CheckerDT_isValid(bIsInitialized, oNRoot, ulCount)); */

   /* validate pcPath and generate a Path_T for it */
   if(!bIsInitialized)
      return INITIALIZATION_ERROR;

   iStatus = Path_new(pcPath, &oPPath);
   if(iStatus != SUCCESS)
      return iStatus;

   /* find the closest ancestor of oPPath already in the tree */
   iStatus= FT_traversePath(oPPath, &oNCurr);
   if(iStatus != SUCCESS)
   {
      Path_free(oPPath);
      return iStatus;
   }

   /* Check whether if parent is a file */
   if (oNCurr == NULL && oNCurr->isFile == TRUE) {
        return NOT_A_DIRECTORY;
     }

   /* no ancestor node found, so if root is not NULL,
      pcPath isn't underneath root. */
   if(oNCurr == NULL && oNRoot != NULL) {
      Path_free(oPPath);
      return CONFLICTING_PATH;
   }

   ulDepth = Path_getDepth(oPPath);
   if(oNCurr == NULL) /* new root! */
      ulIndex = 1;
   else {
      ulIndex = Path_getDepth(Node_getPath(oNCurr))+1;

      /* oNCurr is the node we're trying to insert */
      if(ulIndex == ulDepth+1 && !Path_comparePath(oPPath,
                                       Node_getPath(oNCurr))) {
         Path_free(oPPath);
         return ALREADY_IN_TREE;
      }
   }

   /* starting at oNCurr, build rest of the path one level at a time */
   while(ulIndex <= ulDepth) {
      Path_T oPPrefix = NULL;
      Node_T oNNewNode = NULL;

      /* generate a Path_T for this level */
      iStatus = Path_prefix(oPPath, ulIndex, &oPPrefix);
      if(iStatus != SUCCESS) {
         Path_free(oPPath);
         if(oNFirstNew != NULL)
            (void) Node_free(oNFirstNew);
         /* assert(CheckerDT_isValid(bIsInitialized, oNRoot, ulCount)); */
         return iStatus;
      }

      /* insert the new node for this level */
      iStatus = Node_new(oPPrefix, oNCurr, &oNNewNode, FALSE, NULL, 0);
      if(iStatus != SUCCESS) {
         Path_free(oPPath);
         Path_free(oPPrefix);
         if(oNFirstNew != NULL)
            (void) Node_free(oNFirstNew);
         /* assert(CheckerDT_isValid(bIsInitialized, oNRoot, ulCount)); */
         return iStatus;
      }

      /* set up for next level */
      Path_free(oPPrefix);
      oNCurr = oNNewNode;
      ulNewNodes++;
      if(oNFirstNew == NULL)
         oNFirstNew = oNCurr;
      ulIndex++;
   }

   Path_free(oPPath);
   /* update FT state variables to reflect insertion */
   if(oNRoot == NULL)
      oNRoot = oNFirstNew;
   ulCount += ulNewNodes;

   /* assert(CheckerDT_isValid(bIsInitialized, oNRoot, ulCount)); */
   return SUCCESS;

}

boolean FT_containsFile(const char *pcPath) {
   int iStatus;
   Node_T oNFound = NULL;

   assert(pcPath != NULL);

   iStatus = FT_findNode(pcPath, &oNFound);

   if(oNFound->isFile == FALSE) {
    iStatus = NOT_A_FILE; 
   } 

   return (boolean) (iStatus == SUCCESS);
}

int FT_rmFile(const char *pcPath) {
   int iStatus;
   Node_T oNFound = NULL;

   assert(pcPath != NULL);
   /* assert(CheckerDT_isValid(bIsInitialized, oNRoot, ulCount)); */

   iStatus = DT_findNode(pcPath, &oNFound);

   if(oNFound->isFile == FALSE) {
    return NOT_A_FILE;
   }

   if(iStatus != SUCCESS)
       return iStatus;

   ulCount -= Node_free(oNFound);
   if(ulCount == 0)
      oNRoot = NULL;

   /* assert(CheckerDT_isValid(bIsInitialized, oNRoot, ulCount)); */ 
   return SUCCESS;

}

void *FT_getFileContents(const char *pcPath) {
    int iStatus;
    Node_T oNFound = NULL;

    assert(pcPath != NULL);
   /* assert(CheckerDT_isValid(bIsInitialized, oNRoot, ulCount)); */

   iStatus = FT_findNode(pcPath, &oNFound);

   if(oNFound->isFile == FALSE) {
       return NOT_A_FILE;
   }

   if(iStatus != SUCCESS) {
       return iStatus;
   } 

   return (void*) oNFound->contents; 
}

void *FT_replaceFileContents(const char *pcPath, void *pvNewContents,
                             size_t ulNewLength) {
    const void *pvOldContents; 
    int iStatus;
    Node_T oNFound = NULL;

    assert(pcPath != NULL);
   /* assert(CheckerDT_isValid(bIsInitialized, oNRoot, ulCount)); */

   iStatus = FT_findNode(pcPath, &oNFound);

   if(oNFound->isFile == FALSE) {
       return NOT_A_FILE;
   }

   if(iStatus != SUCCESS) {
       return iStatus;
   } 

   pvOldContents = oNFound->contents;
   oNFound->contents = pvNewContents; 
   oNFound->contentSize = ulNewLength; 
   
   return pvOldContents;    
}

int FT_stat(const char *pcPath, boolean *pbIsFile, size_t *pulSize);

int FT_init(void);

int FT_destroy(void);

char *FT_toString(void);
