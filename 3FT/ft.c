/*--------------------------------------------------------------------*/
/* ft.c                                                               */
/* Author: Kok Wei Pua and Cherie Jiraphanphong                       */
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

/* Questions: */
/* 
Q1: Because the lexicographic order is implemented in dynarray, can we assume 
that it inserts the file and director based on lexicographic order like in 2DT? YES

Q2: How to implement toString? Do we implement in FT or node? See node.c Line 313?

Q4: In node.c Line 169: Because file cannot have children, can we just 
point psNew->oDChildren to NULL? (Yes/no, can ask in Ed)

Q5: Not sure if need to change anything and if iterating through the children of both directory and 
files has an effect on how it iterates through. 

Q8: When we insert a file, can we just copy the logic from the directory? (Can ask in Ed)

Q9: In Line 178: if (oNCurr == NULL && oNCurr->isFile == TRUE) {
Error: pointer to incomplete class type "struct node" is not allowed (Can ask in Ed)

*/





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

static int FT_traversePath(Path_T oPPath, Node_T *poNFurthest) {
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

static int FT_findNode(const char *pcPath, Node_T *poNResult) {
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

   iStatus = FT_traversePath(oPPath, &oNFound);
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
   /* Write a getter function of the file state */
   if (oNCurr != NULL &&  Node_getIsFile(oNCurr) == TRUE) {
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

   if(oNFound != NULL && Node_getIsFile(oNFound) == TRUE) {
    iStatus = NOT_A_DIRECTORY; 
   } 

   return (boolean) (iStatus == SUCCESS);
}

int FT_rmDir(const char *pcPath) {
   int iStatus;
   Node_T oNFound = NULL;

   assert(pcPath != NULL);
   /* assert(CheckerDT_isValid(bIsInitialized, oNRoot, ulCount)); */

   iStatus = FT_findNode(pcPath, &oNFound);

   if(oNFound != NULL && Node_getIsFile(oNFound) == TRUE) {
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

/* Make sure that inserting missing directory before a file*/
/* File cannoy be the first to insert */
int FT_insertFile(const char *pcPath, void *pvContents,
                  size_t ulLength) {
   int iStatus;
   Path_T oPPath = NULL;
   Node_T oNFirstNew = NULL;
   Node_T oNNewNode = NULL;
   Node_T oNCurr = NULL;
   size_t ulDepth, ulIndex;
   size_t ulNewNodes = 0;

   assert(pcPath != NULL);
   /* assert(CheckerDT_isValid(bIsInitialized, oNRoot, ulCount)); */

   /* validate pcPath */
   if(!bIsInitialized)
      return INITIALIZATION_ERROR;
   
   /* Generate a Path_T for pcPath */
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

   /* no ancestor node found, so if root is not NULL,
      pcPath isn't underneath root. */
   if(oNCurr == NULL && oNRoot != NULL) {
      Path_free(oPPath);
      return CONFLICTING_PATH;
   }

   ulDepth = Path_getDepth(oPPath);

   /* cannot insert file at the root. */
   if(oNCurr == NULL && ulDepth == 1) {
      return CONFLICTING_PATH;
   }
   
   /* Check whether if parent is a file */
   if (oNCurr != NULL && Node_getIsFile(oNCurr) == FALSE) {
        return NOT_A_FILE;
   }

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
   while(ulIndex < ulDepth) {
      Path_T oPPrefix = NULL;
      Node_T oNPrefixNewNode = NULL;

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
      iStatus = Node_new(oPPrefix, oNCurr, &oNPrefixNewNode, FALSE, NULL, 0);
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
      oNCurr = oNPrefixNewNode;
      ulNewNodes++;
      if(oNFirstNew == NULL)
         oNFirstNew = oNCurr;
      ulIndex++;
   }

   /* Insert file new node */
   iStatus = Node_new(oPPath, oNCurr, &oNNewNode, TRUE, pvContents, ulLength);
   if(iStatus != SUCCESS) {
      Path_free(oPPath);
      if(oNFirstNew != NULL)
         (void) Node_free(oNFirstNew);
      /* assert(CheckerDT_isValid(bIsInitialized, oNRoot, ulCount)); */
      return iStatus;
   }

   oNCurr = oNNewNode;
   ulNewNodes++;
   if(oNFirstNew == NULL)
      oNFirstNew = oNCurr;
   ulIndex++;

   oNCurr = oNNewNode;
   ulNewNodes++;
   ulIndex++;

   Path_free(oPPath);
   /* update DT state variables to reflect insertion */
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

   if(oNFound != NULL && Node_getIsFile(oNFound) == FALSE) {
    iStatus = NOT_A_FILE; 
   } 

   return (boolean) (iStatus == SUCCESS);
}

int FT_rmFile(const char *pcPath) {
   int iStatus;
   Node_T oNFound = NULL;

   assert(pcPath != NULL);
   /* assert(CheckerDT_isValid(bIsInitialized, oNRoot, ulCount)); */

   iStatus = FT_findNode(pcPath, &oNFound);

   if(oNFound != NULL && Node_getIsFile(oNFound) == FALSE) {
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

   if(Node_getIsFile(oNFound) == FALSE) {
       return NULL;
   }

   if(iStatus != SUCCESS) {
      return NULL;
   } 

   return (void*)Node_getFileContents(oNFound); 
}

void *FT_replaceFileContents(const char *pcPath, void *pvNewContents,
                             size_t ulNewLength) { 
    int iStatus;
    Node_T oNFound = NULL;

    assert(pcPath != NULL);
   /* assert(CheckerDT_isValid(bIsInitialized, oNRoot, ulCount)); */

   iStatus = FT_findNode(pcPath, &oNFound);

   if(Node_getIsFile(oNFound) == FALSE) {
       return NULL;
   }

   if(iStatus != SUCCESS) {
       return NULL;
   } 

   return (void*)Node_replaceFileContents(oNFound, pvNewContents, ulNewLength);     
}

int FT_stat(const char *pcPath, boolean *pbIsFile, size_t *pulSize) {
   int iStatus;
   Node_T oNFound = NULL;

   assert(pcPath != NULL);
   assert(pbIsFile != NULL);
   assert(pulSize != NULL);
   /* assert(CheckerDT_isValid(bIsInitialized, oNRoot, ulCount)); */

   iStatus = FT_findNode(pcPath, &oNFound);

   if(Node_getIsFile(oNFound) == TRUE) {
      *pbIsFile = TRUE; 
      *pulSize = Node_getContentLength(oNFound);
   }
   else {
      *pbIsFile = FALSE; 
   }
   
   return (boolean) (iStatus == SUCCESS);
}

int FT_init(void) {
   /* assert(CheckerDT_isValid(bIsInitialized, oNRoot, ulCount)); */ 

   if(bIsInitialized)
      return INITIALIZATION_ERROR;

   bIsInitialized = TRUE;
   oNRoot = NULL;
   ulCount = 0;

   /* assert(CheckerDT_isValid(bIsInitialized, oNRoot, ulCount)); */ 
   return SUCCESS;
}

int FT_destroy(void) {
   /* assert(CheckerDT_isValid(bIsInitialized, oNRoot, ulCount));*/

   if(!bIsInitialized)
      return INITIALIZATION_ERROR;

   if(oNRoot) {
      ulCount -= Node_free(oNRoot);
      oNRoot = NULL;
   }

   bIsInitialized = FALSE;

   /* assert(CheckerDT_isValid(bIsInitialized, oNRoot, ulCount)); */ 
   return SUCCESS;
}

/* --------------------------------------------------------------------

  The following auxiliary functions are used for generating the
  string representation of the DT.
*/

/*
  Performs a pre-order traversal of the tree rooted at n,
  inserting each payload to DynArray_T d beginning at index i.
  Returns the next unused index in d after the insertion(s).
*/

/* Add the file for loop not nested */ 
static size_t FT_preOrderTraversal(Node_T n, DynArray_T d, size_t i) {
   size_t c;

   assert(d != NULL);

   if(n != NULL) {
      (void) DynArray_set(d, i, n);
      i++;
      for(c = 0; c < Node_getNumChildren(n); c++) {
         int iStatus;
         Node_T oNChild = NULL;
         iStatus = Node_getChild(n,c, &oNChild);
         assert(iStatus == SUCCESS);
         if(Node_getIsFile(oNChild) == TRUE) {
            DynArray_set(d, i, oNChild);
            i++;
         }
      }
      
      for(c = 0; c < Node_getNumChildren(n); c++) {
         int iStatus;
         Node_T oNChild = NULL;
         iStatus = Node_getChild(n,c, &oNChild);
         assert(iStatus == SUCCESS);
         if(Node_getIsFile(oNChild) == FALSE) {
            i = FT_preOrderTraversal(oNChild, d, i);
         }
      }
   }
   return i;
}

/*
  Alternate version of strlen that uses pulAcc as an in-out parameter
  to accumulate a string length, rather than returning the length of
  oNNode's path, and also always adds one addition byte to the sum.
*/
static void FT_strlenAccumulate(Node_T oNNode, size_t *pulAcc) {
   assert(pulAcc != NULL);

   if(oNNode != NULL)
      *pulAcc += (Path_getStrLength(Node_getPath(oNNode)) + 1);
}

/*
  Alternate version of strcat that inverts the typical argument
  order, appending oNNode's path onto pcAcc, and also always adds one
  newline at the end of the concatenated string.
*/
static void FT_strcatAccumulate(Node_T oNNode, char *pcAcc) {
   assert(pcAcc != NULL);

   if(oNNode != NULL) {
      strcat(pcAcc, Path_getPathname(Node_getPath(oNNode)));
      strcat(pcAcc, "\n");
   }
}
/*--------------------------------------------------------------------*/

char *FT_toString(void) {
   DynArray_T nodes;
   size_t totalStrlen = 1;
   char *result = NULL;

   if(!bIsInitialized)
      return NULL;
   
   nodes = DynArray_new(ulCount);
   (void) FT_preOrderTraversal(oNRoot, nodes, 0);

   DynArray_map(nodes, (void (*)(void *, void*)) FT_strlenAccumulate,
                (void*) &totalStrlen);

   result = malloc(totalStrlen);
   if(result == NULL) {
      DynArray_free(nodes);
      return NULL;
   }
   *result = '\0';

   DynArray_map(nodes, (void (*)(void *, void*)) FT_strcatAccumulate,
                (void *) result);

   DynArray_free(nodes);

   return result;
}
