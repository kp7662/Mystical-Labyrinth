/*--------------------------------------------------------------------*/
/* checkerDT.c                                                        */
/* Author:                                                            */
/*--------------------------------------------------------------------*/

/* How we do check if we have eveything we need to change.  */

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "checkerDT.h"
#include "dynarray.h"
#include "path.h"



/* see checkerDT.h for specification */
boolean CheckerDT_Node_isValid(Node_T oNNode) {
   Node_T oNParent;
   Path_T oPNPath;
   Path_T oPPPath;

   /* Sample check: a NULL pointer is not a valid node */
   if(oNNode == NULL) {
      fprintf(stderr, "A node is a NULL pointer\n");
      return FALSE;
   }

   /* Sample check: parent's path must be the longest possible
      proper prefix of the node's path */
   oNParent = Node_getParent(oNNode);
   if(oNParent != NULL) {
      oPNPath = Node_getPath(oNNode);
      oPPPath = Node_getPath(oNParent);

      if(Path_getSharedPrefixDepth(oPNPath, oPPPath) !=
         Path_getDepth(oPNPath) - 1) {
         fprintf(stderr, "P-C nodes don't have P-C paths: (%s) (%s)\n",
                 Path_getPathname(oPPPath), Path_getPathname(oPNPath));
         return FALSE;
      }
   } 

   return TRUE;
}

/*
   Performs a pre-order traversal of the tree rooted at oNNode.
   Returns FALSE if a broken invariant is found and
   returns TRUE otherwise.

   You may want to change this function's return type or
   parameter list to facilitate constructing your checks.
   If you do, you should update this function comment.

   change to iclude pointer to the address of the counter 
*/
static boolean CheckerDT_treeCheck(Node_T oNNode, size_t *pcounter) {
   size_t ulIndex;

   if(oNNode!= NULL) {
      Node_T oNChildprev = NULL;
      /* Sample check on each node: node must be valid */
      /* If not, pass that failure back up immediately */
      if(!CheckerDT_Node_isValid(oNNode))
         return FALSE;
      
      *pcounter = (*pcounter) + 1;

      /* Recur on every child of oNNode */
      for(ulIndex = 0; ulIndex < Node_getNumChildren(oNNode); ulIndex++)
      {
         /* How do we check that the children don't have the same name and if its in lexographic order --> we do*/ 
         Node_T oNChild = NULL;

         int iStatus = Node_getChild(oNNode, ulIndex, &oNChild); 

         if(iStatus != SUCCESS) {
            fprintf(stderr, "getNumChildren claims more children than getChild returns\n");
            return FALSE;
         }

         if(oNChildprev != NULL) {
            if(Path_comparePath(Node_getPath(oNChild), Node_getPath(oNChildprev)) < 0) {
               fprintf(stderr, "The children are not in lexicographic order\n");
               return FALSE; 
            } 
            if(Path_comparePath(Node_getPath(oNChild), Node_getPath(oNChildprev)) == 0) {
               fprintf(stderr, "There are duplicates in the node's children\n");
               return FALSE; 
            } 
         }
         /* if recurring down one subtree results in a failed check
            farther down, passes the failure back up immediately */
            /* make sure its the same as signiture */
         if(!CheckerDT_treeCheck(oNChild, pcounter))
            return FALSE;
         
         oNChildprev = oNChild; 
      }
   }

   
   return TRUE;
}

/* see checkerDT.h for specification */
/* do we have to check if the tree has to root here */
boolean CheckerDT_isValid(boolean bIsInitialized, Node_T oNRoot,
                          size_t ulCount) {

   size_t counter = 0; 
   boolean iStatus; 
   
   /* Sample check on a top-level data structure invariant:
      if the DT is not initialized, its count should be 0. */
   if(!bIsInitialized) {
      if(ulCount != 0) {
         fprintf(stderr, "Not initialized, but count is not 0\n");
         return FALSE;
      }
      /* if DT is not initialized, its pointer to root node should be NULL. */
      if(oNRoot != NULL) {
         fprintf(stderr, "Not initialized, but pointer to root node is not NULL\n");
         return FALSE;
      }
   }
   else {
      /* If have non-null root then count cannot be 0 --> this is root */
      if (oNRoot != NULL && ulCount == 0) {
         fprintf(stderr, "Initialized and pointer to root node is not NULL, but count is still 0\n");
         return FALSE;
      } 

      if (oNRoot != NULL) {
         /*  Parent of root node has to be NULL. */
         if(Node_getParent(oNRoot) != NULL) {
            fprintf(stderr, "Initialized, but parent of root node is not equal to NULL\n");
            return FALSE;
         } 
      } 

      /* Now checks invariants recursively at each node from the root. */
      /* Need a counter tocount the number of node that tree goes true, send a pointer to the recurisive function */
      iStatus = CheckerDT_treeCheck(oNRoot, &counter);

      if (iStatus == FALSE) return FALSE; 
      
      if (iStatus == TRUE && counter != ulCount) {
         fprintf(stderr, "Initialized, but count is not equal to the number of nodes in the tree\n");
         return FALSE; 
      } 
   } 

   return TRUE; 
}
