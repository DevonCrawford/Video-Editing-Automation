/**
 * LinkedListAPI.c
 * Devon Crawford
 * February 2, 2018
 */

#include "LinkedListAPI.h"

/** Function to initialize the list metadata head to the appropriate function pointers. Allocates memory to the struct.
*@return pointer to the list head
*@param printFunction function pointer to print a single node of the list
*@param deleteFunction function pointer to delete a single piece of data from the list
*@param compareFunction function pointer to compare two nodes of the list in order to test for equality or order
**/
List initializeList(char* (*printFunction)(void* toBePrinted),void (*deleteFunction)(void* toBeDeleted),int (*compareFunction)(const void* first,const void* second)){
	List tmpList;

	tmpList.head = NULL;
	tmpList.tail = NULL;
	tmpList.length = 0;
	tmpList.deleteData = deleteFunction;
	tmpList.compare = compareFunction;
	tmpList.printData = printFunction;

	return tmpList;
}

/**Function for creating a node for the linked list.
* This node contains abstracted (void *) data as well as previous and next
* pointers to connect to other nodes in the list
*@pre data should be of same size of void pointer on the users machine to avoid size conflicts. data must be valid.
*data must be cast to void pointer before being added.
*@post data is valid to be added to a linked list
*@return On success returns a node that can be added to a linked list. On failure, returns NULL.
*@param data - is a void * pointer to any data type.  Data must be allocated on the heap.
**/
Node* initializeNode(void* data){
	Node* tmpNode;

	tmpNode = (Node*)malloc(sizeof(Node));

	if (tmpNode == NULL){
		return NULL;
	}

	tmpNode->data = data;
	tmpNode->previous = NULL;
	tmpNode->next = NULL;

	return tmpNode;
}

/**Inserts a Node at the front of a linked list.  List metadata is updated
* so that head and tail pointers are correct.
*@pre 'List' type must exist and be used in order to keep track of the linked list.
*@param list pointer to the dummy head of the list
*@param toBeAdded a pointer to data that is to be added to the linked list
**/
void insertFront(List* list, void* toBeAdded){
	if (list == NULL || toBeAdded == NULL){
		return;
	}

	Node* newNode = initializeNode(toBeAdded);

    if (list->head == NULL && list->tail == NULL){
        list->head = newNode;
        list->tail = list->head;
    }else{
		newNode->next = list->head;
        list->head->previous = newNode;
    	list->head = newNode;
    }
	(list->length)++;
}

/**Inserts a Node at the back of a linked list.  List metadata is updated
* so that head and tail pointers are correct.
*@pre 'List' type must exist and be used in order to keep track of the linked list.
*@param list pointer to the dummy head of the list
*@param toBeAdded a pointer to data that is to be added to the linked list
**/
void insertBack(List* list, void* toBeAdded){
	if (list == NULL || toBeAdded == NULL){
		return;
	}

	Node* newNode = initializeNode(toBeAdded);

    if (list->head == NULL && list->tail == NULL){
        list->head = newNode;
        list->tail = list->head;
    }else{
		newNode->previous = list->tail;
        list->tail->next = newNode;
    	list->tail = newNode;
    }
	(list->length)++;
}

/** Deletes the entire linked list, freeing all memory.
* uses the supplied function pointer to release allocated memory for the data
* returns NULL on success or the head of the list on failure.
*@pre 'List' type must exist and be used in order to keep track of the linked list.
*@param list pointer to the List-type dummy node
*@return  on success: NULL, on failure: head of list
**/
void clearList(List* list){

    if (list == NULL){
		return;
	}

	if (list->head == NULL && list->tail == NULL){
		return;
	}

	Node* tmp;

	while (list->head != NULL){
		list->deleteData(list->head->data);
		tmp = list->head;
		list->head = list->head->next;
		free(tmp);
		tmp = NULL;
	}

	list->head = NULL;
	list->tail = NULL;
}

/** Uses the comparison function pointer to place the element in the
* appropriate position in the list.
* should be used as the only insert function if a sorted list is required.
*@pre List exists and has memory allocated to it. Node to be added is valid.
*@post The node to be added will be placed immediately before or after the first occurrence of a related node
*@param list a pointer to the dummy head of the list containing function pointers for delete and compare, as well
as a pointer to the first and last element of the list.
*@param toBeAdded a pointer to data that is to be added to the linked list
**/
void insertSorted(List *list, void *toBeAdded){
	if (list == NULL || toBeAdded == NULL){
		return;
	}

	if (list->head == NULL){
		insertBack(list, toBeAdded);
		return;
	}

	if (list->compare(toBeAdded, list->head->data) <= 0){
		insertFront(list, toBeAdded);
		return;
	}

	if (list->compare(toBeAdded, list->tail->data) > 0){
		insertBack(list, toBeAdded);
		return;
	}

	Node* currNode = list->head;

	while (currNode != NULL){
		if (list->compare(toBeAdded, currNode->data) <= 0){
			Node* newNode = initializeNode(toBeAdded);
			newNode->next = currNode;
			newNode->previous = currNode->previous;
			currNode->previous->next = newNode;
			currNode->previous = newNode;
			(list->length)++;
			return;
		}
		currNode = currNode->next;
	}

	return;
}

/** Uses the comparison function pointer to place the element in the
* appropriate position in the list. Also return the node of inserted element
* should be used as the only insert function if a sorted list is required.
*@pre List exists and has memory allocated to it. Node to be added is valid.
*@post The node to be added will be placed immediately before or after the first occurrence of a related node
*@param list a pointer to the dummy head of the list containing function pointers for delete and compare, as well
as a pointer to the first and last element of the list.
*@param toBeAdded a pointer to data that is to be added to the linked list
**/
Node *insertSortedGetNode(List *list, void *toBeAdded){
	if (list == NULL || toBeAdded == NULL){
		return NULL;
	}

	if (list->head == NULL){
		insertBack(list, toBeAdded);
		return list->head;
	}

	if (list->compare(toBeAdded, list->head->data) <= 0){
		insertFront(list, toBeAdded);
		return list->head;
	}

	if (list->compare(toBeAdded, list->tail->data) > 0){
		insertBack(list, toBeAdded);
		return list->tail;
	}

	Node* currNode = list->head;

	while (currNode != NULL){
		if (list->compare(toBeAdded, currNode->data) <= 0){
			Node* newNode = initializeNode(toBeAdded);
			newNode->next = currNode;
			newNode->previous = currNode->previous;
			currNode->previous->next = newNode;
			currNode->previous = newNode;
			(list->length)++;
			return newNode;
		}
		currNode = currNode->next;
	}
	return NULL;
}

void* deleteDataFromList(List* list, void* toBeDeleted){
	if (list == NULL || toBeDeleted == NULL){
		return NULL;
	}
	Node* tmp = list->head;

	while(tmp != NULL){
		if (list->compare(toBeDeleted, tmp->data) == 0){
			//Unlink the node
			Node* delNode = tmp;

			if (tmp->previous != NULL){
				tmp->previous->next = delNode->next;
			}else{
				list->head = delNode->next;
			}

			if (tmp->next != NULL){
				tmp->next->previous = delNode->previous;
			}else{
				list->tail = delNode->previous;
			}

			void* data = delNode->data;
			// list->deleteData(data);
			free(delNode);
			(list->length)--;

			return data;
		}else{
			tmp = tmp->next;
		}
	}
	return NULL;
}

/**Returns a pointer to the data at the front of the list. Does not alter list structure.
 *@pre The list exists and has memory allocated to it
 *@param the list struct
 *@return pointer to the data located at the head of the list
 **/
void* getFromFront(List list){

	if (list.head == NULL){
		return NULL;
	}

	return list.head->data;
}

/**Returns a pointer to the data at the back of the list. Does not alter list structure.
 *@pre The list exists and has memory allocated to it
 *@param the list struct
 *@return pointer to the data located at the tail of the list
 **/
void* getFromBack(List list){

	if (list.tail == NULL){
		return NULL;
	}

	return list.tail->data;

}

/**
 * Return the Node containing search data
 * @param  list List
 * @param  data generic data to be searched using compare function
 * @return      Node * on success, NULL on fail
 */
Node *getNodeFromData(List *list, void *data) {
	Node *curr = list->head;

	while(curr != NULL) {
		if(list->compare(data, curr->data) == 0) {
			return curr;
		}
		curr = curr->next;
	}
	return NULL;
}

/**Returns a string that contains a string representation of the list traversed from  head to tail.
Utilize an iterator and the list's printData function pointer to create the string.
returned string must be freed by the calling function.
 *@pre List must exist, but does not have to have elements.
 *@param list Pointer to linked list dummy head.
 *@return on success: char * to string representation of list (must be freed after use).  on failure: NULL
 **/
char* toString(List list){
	ListIterator iter = createIterator(list);
	char* str;

	str = (char*)malloc(sizeof(char));
	strcpy(str, "");

	void* elem;
	while( (elem = nextElement(&iter)) != NULL){
		char* currDescr = list.printData(elem);
		int newLen = strlen(str)+50+strlen(currDescr);
		str = (char*)realloc(str, newLen);
		strcat(str, "\n");
		strcat(str, currDescr);
		free(currDescr);
	}

	return str;
}

ListIterator createIterator(List list){
    ListIterator iter;

    iter.current = list.head;

    return iter;
}

void* nextElement(ListIterator* iter){
    Node* tmp = iter->current;

    if (tmp != NULL){
        iter->current = iter->current->next;
        return tmp->data;
    }else{
        return NULL;
    }
}

/**
 * Seek the list iterator to a specific index
 * @param  iter  ListIterator containing current node (where seek node will be saved)
 * @param  index position to seek (starting at 0)
 * @return       >= 0 on success
 */
int seekIterator(ListIterator* iter, int index) {
	Node *tmp = iter->current;
	int i;
	for(i = 0; tmp != NULL && i < index; i++) {
		tmp = tmp->next;
	}
	if(tmp != NULL && i == index) {
		iter->current = tmp;
		return 0;
	} else {
		return -1;
	}
}

/**Returns the number of elements in the list.
 *@pre List must exist, but does not have to have elements.
 *@param list - the list struct.
 *@return on success: number of eleemnts in the list (0 or more).  on failure: -1 (e.g. list not initlized correctly)
 **/
int getLength(List list) {
	return list.length;
}

/** Function that searches for an element in the list using a comparator function.
 * If an element is found, a pointer to the data of that element is returned
 * Returns NULL if the element is not found.
 *@pre List exists and is valid.  Comparator function has been provided.
 *@post List remains unchanged.
 *@return The data associated with the list element that matches the search criteria.  If element is not found, return NULL.
 *@param list - a list sruct
 *@param customCompare - a pointer to comparator fuction for customizing the search
 *@param searchRecord - a pointer to search data, which contains seach criteria
 *Note: while the arguments of compare() and searchRecord are all void, it is assumed that records they point to are
 *      all of the same type - just like arguments to the compare() function in the List struct
 **/
void* findElement(List list, bool (*customCompare)(const void* first,const void* second), const void* searchRecord) {
	Node *curr = list.head;

	while(curr != NULL) {
		if(customCompare(curr->data, searchRecord) == true) {
			return (void *)curr->data;
		}
		curr = curr->next;
	}
	return NULL;
}
