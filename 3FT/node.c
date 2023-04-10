/*--------------------------------------------------------------------*/
/* nodeDT.c                                                           */
/* Author: Kok Wei Pua and Cherie Jiraphanphong                       */
/*--------------------------------------------------------------------*/

/* Do we need to put child in lexicographic order through Node*/

#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "dynarray.h"
#include "node.h"
#include "checkerDT.h"

/* Can we assume contents is string? */

/* A node in a FT (can either be a file or a directory) */
struct node {
   /* the object corresponding to the node's absolute path */
   Path_T oPPath;
   /* this node's parent */
   Node_T oNParent;
   /* the object containing links to this node's children */
   DynArray_T oDChildren;

    /* check if the node is a file */
    boolean isFile;
    /* contents of the file node */
    void *contents;
    /* content size */
    size_t contentSize;
};


/*
  Links new child oNChild into oNParent's children array at index
  ulIndex. Returns SUCCESS if the new child was added successfully,
  or  MEMORY_ERROR if allocation fails adding oNChild to the array.
*/
static int Node_addChild(Node_T oNParent, Node_T oNChild,
                         size_t ulIndex) {
   assert(oNParent != NULL);
   assert(oNChild != NULL);

   if(DynArray_addAt(oNParent->oDChildren, ulIndex, oNChild))
      return SUCCESS;
   else
      return MEMORY_ERROR;
}

/*
  Compares the string representation of oNfirst with a string
  pcSecond representing a node's path.
  Returns <0, 0, or >0 if oNFirst is "less than", "equal to", or
  "greater than" pcSecond, respectively.
*/
static int Node_compareString(const Node_T oNFirst,
                                 const char *pcSecond) {
   assert(oNFirst != NULL);
   assert(pcSecond != NULL);

   return Path_compareString(oNFirst->oPPath, pcSecond);
}


/*
  Creates a new node with path oPPath and parent oNParent.  Returns an
  int SUCCESS status and sets *poNResult to be the new node if
  successful. Otherwise, sets *poNResult to NULL and returns status:
  * MEMORY_ERROR if memory could not be allocated to complete request
  * CONFLICTING_PATH if oNParent's path is not an ancestor of oPPath
  * NO_SUCH_PATH if oPPath is of depth 0
                 or oNParent's path is not oPPath's direct parent
                 or oNParent is NULL but oPPath is not of depth 1
  * ALREADY_IN_TREE if oNParent already has a child with this path
*/
int Node_new(Path_T oPPath, Node_T oNParent, Node_T *poNResult, boolean isFile, void *contents, size_t contentSize) {
   /* Intialize all arguments */
   struct node *psNew;
   Path_T oPParentPath = NULL;
   Path_T oPNewPath = NULL;
   size_t ulParentDepth;
   size_t ulIndex;
   int iStatus;

   assert(oPPath != NULL);
   /* assert(oNParent == NULL || CheckerDT_Node_isValid(oNParent)); */ /* Do we need this? */

   /* allocate space for a new node */
   psNew = malloc(sizeof(struct node));
   if(psNew == NULL) {
      *poNResult = NULL;
      return MEMORY_ERROR;
   }

   /* set the new node's path */
   iStatus = Path_dup(oPPath, &oPNewPath);
   if(iStatus != SUCCESS) {
      free(psNew);
      *poNResult = NULL;
      return iStatus;
   }
   psNew->oPPath = oPNewPath;

   /* validate and set the new node's parent */
   if(oNParent != NULL) {
      size_t ulSharedDepth;

      oPParentPath = oNParent->oPPath;
      ulParentDepth = Path_getDepth(oPParentPath);
      ulSharedDepth = Path_getSharedPrefixDepth(psNew->oPPath,
                                                oPParentPath);
      /* parent must be an ancestor of child */
      if(ulSharedDepth < ulParentDepth) {
         Path_free(psNew->oPPath);
         free(psNew);
         *poNResult = NULL;
         return CONFLICTING_PATH;
      }

      /* parent must be exactly one level up from child */
      if(Path_getDepth(psNew->oPPath) != ulParentDepth + 1) {
         Path_free(psNew->oPPath);
         free(psNew);
         *poNResult = NULL;
         return NO_SUCH_PATH;
      }

      /* parent must not already have child with this path */
      if(oNParent->isFile == FALSE) {
         if(Node_hasChild(oNParent, oPPath, &ulIndex)) {
            Path_free(psNew->oPPath);
            free(psNew);
            *poNResult = NULL;
            return ALREADY_IN_TREE;
         }
      } 
      else {
            Path_free(psNew->oPPath);
            free(psNew);
            *poNResult = NULL;
            return NOT_A_DIRECTORY; 
      }
    }
    
    else {
      /* new node must be root */
      /* can only create one "level" at a time */
      if(Path_getDepth(psNew->oPPath) != 1) {
         Path_free(psNew->oPPath);
         free(psNew);
         *poNResult = NULL;
         return NO_SUCH_PATH;
      }
   }
   psNew->oNParent = oNParent;

   /* initialize the new node */
   psNew->isFile = isFile;
   if(psNew->isFile == TRUE) {
      /* Need to consider what happen when contents = NULL. */
      if(contents == NULL) {
         psNew->contents = NULL;
         psNew->contentSize = 0; 
      }
      else {
         /* Line 169: Because file cannot have children, can we just point psNew->oDChildren to NULL? */
         psNew->contents = contents; 
         psNew->contentSize = contentSize; 
         psNew->oDChildren = NULL; /* Double check */
      }
   }
    /* if new node is a directory */
   else {
      psNew->oDChildren = DynArray_new(0);
      if(psNew->oDChildren == NULL) {
         Path_free(psNew->oPPath);
         free(psNew);
         *poNResult = NULL;
         return MEMORY_ERROR;
      }
   }

   /* Link into parent's children list */
   if(oNParent != NULL) {
      iStatus = Node_addChild(oNParent, psNew, ulIndex);
      if(iStatus != SUCCESS) {
         Path_free(psNew->oPPath);
         free(psNew);
         *poNResult = NULL;
         return iStatus;
      }
   }

   *poNResult = psNew;

    /* Do we need CheckerDT_Node_isValid? Do we need to write our own checker function? */
      /* assert(oNParent == NULL || CheckerDT_Node_isValid(oNParent)); */
      /* assert(CheckerDT_Node_isValid(*poNResult)); */ 

   return SUCCESS;
}

size_t Node_free(Node_T oNNode) {
   size_t ulIndex;
   size_t ulCount = 0;

   assert(oNNode != NULL);
   /* assert(CheckerDT_Node_isValid(oNNode)); */

   /* remove from parent's list */
   if(oNNode->oNParent != NULL) {
      if(DynArray_bsearch(
            oNNode->oNParent->oDChildren,
            oNNode, &ulIndex,
            (int (*)(const void *, const void *)) Node_compare)
        )
         (void) DynArray_removeAt(oNNode->oNParent->oDChildren,
                                  ulIndex);
   }

   /* recursively remove children */
   if (oNNode->isFile == FALSE) {
        while(DynArray_getLength(oNNode->oDChildren) != 0) {
            ulCount += Node_free(DynArray_get(oNNode->oDChildren, 0));
        }
        DynArray_free(oNNode->oDChildren);
   }
   else {
      DynArray_free(oNNode->oDChildren);
   }
   
   /* remove path */
   Path_free(oNNode->oPPath);

   /* finally, free the struct node */
   free(oNNode);
   ulCount++;
   return ulCount;
}

Path_T Node_getPath(Node_T oNNode) {
   assert(oNNode != NULL);

   return oNNode->oPPath;
}

boolean Node_hasChild(Node_T oNParent, Path_T oPPath,
                         size_t *pulChildID) {
   assert(oNParent != NULL);
   assert(oPPath != NULL);
   assert(pulChildID != NULL);

   /* If the node is a file. */
   if (oNParent->isFile == TRUE) {
      return FALSE; 
   }
   
   /* *pulChildID is the index into oNParent->oDChildren */
   return DynArray_bsearch(oNParent->oDChildren,
            (char*) Path_getPathname(oPPath), pulChildID,
            (int (*)(const void*,const void*)) Node_compareString);
}

size_t Node_getNumChildren(Node_T oNParent) {
   assert(oNParent != NULL);

   /* If node is a file */
   if (oNParent->isFile == TRUE) {
      return 0; 
   }

   return DynArray_getLength(oNParent->oDChildren);
}

int Node_getChild(Node_T oNParent, size_t ulChildID,
                   Node_T *poNResult) {

   assert(oNParent != NULL);
   assert(poNResult != NULL);

   /* If node is a file. */
   if(oNParent->isFile == TRUE) {
      return NO_SUCH_PATH; 
   }
   
   /* ulChildID is the index into oNParent->oDChildren */
   if(ulChildID >= Node_getNumChildren(oNParent)) {
      *poNResult = NULL;
      return NO_SUCH_PATH;
   }
   else {
      *poNResult = DynArray_get(oNParent->oDChildren, ulChildID);
      return SUCCESS;
   }
}

Node_T Node_getParent(Node_T oNNode) {
   assert(oNNode != NULL);

   return oNNode->oNParent;
}

int Node_compare(Node_T oNFirst, Node_T oNSecond) {
   assert(oNFirst != NULL);
   assert(oNSecond != NULL);

   return Path_comparePath(oNFirst->oPPath, oNSecond->oPPath);
}

/* Not sure if need to change anything and if iterating through the children of both directory and 
files has an effect on how it iterates through. */
char *Node_toString(Node_T oNNode) {
   char *copyPath;

   assert(oNNode != NULL);

   copyPath = malloc(Path_getStrLength(Node_getPath(oNNode))+1);
   if(copyPath == NULL)
      return NULL;
   else
      return strcpy(copyPath, Path_getPathname(Node_getPath(oNNode)));
}

void *Node_replaceFileContents(Node_T oNNode, void *pvContents, size_t newContentSize) {
   const void *pvOldContents; 
   void *pvNewContent;

   if (oNNode->isFile == FALSE) {
      return NULL; 
   }
   
   pvOldContents = oNNode->contents;
   
   pvNewContent = (void *)malloc(sizeof(newContentSize));
   if(pvNewContent == NULL) {
      return MEMORY_ERROR; 
   }
   else {
      pvNewContent = strcpy(pvNewContent, pvContents);
      oNNode->contents = pvNewContent;
      oNNode->contentSize = newContentSize;
   }
   return pvOldContents; 
}

void *Node_getFileContents(Node_T oNNode) {
   if (oNNode->isFile == FALSE) {
      return NULL; 
   }

   return oNNode->contents;
}

size_t Node_getContentLength(Node_T oNNode) {
   if (oNNode->isFile == FALSE) {
      return 0; /* should it be 0 or null */ 
   }

   return oNNode->contentSize; 
}