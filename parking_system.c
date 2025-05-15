#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>
#include <time.h>

#define VACANT 0
#define OCCUPIED 1

// B+ Tree Order and Limits
#define ORDER 5
#define MAXKEYS (ORDER - 1)
#define MINKEYS (MAXKEYS / 2)
#define MAXCHILDREN ORDER

typedef enum { FAILURE, SUCCESS } status_code;
typedef enum { NOTPARKED, PARKED } parked; // for user


typedef struct User_Node 
{
    char vehicle_num[20]; // Primary Key
    char owner_name[50];
    char arrival_date[11];
    char departure_date[11];
    char arrival_time[6];
    char departure_time[6];
    float spent_time;
    float total_spent_time;
    int membership;
    int number_of_parkings;
    int parking_space_id;
    float parking_amt;
    float total_parking_amt;
    parked status;

} User;

typedef struct Parking_Node 
{
    int parking_id; // Primary Key
    int parking_space_status;
    float revenue;
    int occupancies;

} Parking;

typedef struct GenericBPlusTreeNode 
{
    void* keys[MAXKEYS]; // Internal: Routing keys (copies). Leaf: Pointers to data records (User* or Parking*).
    struct GenericBPlusTreeNode* children[MAXCHILDREN]; // Internal: Pointers to children. Leaf: Not used (NULL).
    struct GenericBPlusTreeNode* parent;
    struct GenericBPlusTreeNode* next_leaf;
    int num_keys;
    bool is_leaf;

} GenericBPlusTreeNode;

typedef struct ListNode 
{
    void* data;
    struct ListNode* next;

} ListNode;

// Function Pointer Types
typedef int (*CompareDataFunc)(const void* key1, const void* key2);
typedef int (*CompareInternalKeyFunc)(const void* key1, const void* key2);
typedef void (*PrintFunc)(const void* data);
typedef void (*PrintFuncFile)(const void* data, FILE* file);
typedef void (*FreeDataFunc)(void* data);
typedef void* (*GetKeyFromDataFunc)(const void* data);
typedef void* (*CopyKeyFunc)(const void* key);
typedef void (*FreeKeyFunc)(void* key);

User* createUser(const char* vehicle_num, const char* owner_name, const char* arrival_date, const char* arrival_time, int parking_id) 
{
    User* nptr = (User*)malloc(sizeof(User));

    if (!nptr) 
    {
        perror("Memory Allocation Failed for User");
        exit(EXIT_FAILURE);
    }

    strcpy(nptr->vehicle_num, vehicle_num);
    strcpy(nptr->owner_name, owner_name);
    strcpy(nptr->arrival_date, arrival_date);
    strcpy(nptr->arrival_time, arrival_time);
    strcpy(nptr->departure_date, "-");
    strcpy(nptr->departure_time, "-");
    nptr->parking_space_id = (parking_id > 0) ? parking_id : -1; // Mark invalid if not assigned yet
    nptr->number_of_parkings = 1;
    nptr->membership = 0;
    nptr->spent_time = 0;
    nptr->total_spent_time = 0;
    nptr->parking_amt = 0;
    nptr->total_parking_amt = 0;
    nptr->status = (parking_id > 0) ? PARKED : NOTPARKED; // Status depends if slot assigned

    return nptr;
}

Parking* createParkingSlot(int i) 
{
    Parking* nptr = (Parking*)malloc(sizeof(Parking));

    if (!nptr) 
    {
        perror("Memory Allocation Failed for Parking");
        exit(EXIT_FAILURE);
    }

    nptr->parking_id = i;
    nptr->occupancies = 0;
    nptr->parking_space_status = VACANT;
    nptr->revenue = 0;

    return nptr;
}

void freeUser(void* data) 
{
    free(data);
}

void freeParking(void* data) 
{
    free(data);
}


// User: Compare vehicle_num (char*) with User* data
int compareUserVehicleNum(const void* key, const void* data) 
{
    return strcmp((const char*)key, ((const User*)data)->vehicle_num);
}

// User: Compare vehicle_num (char*) with internal key (char*)
int compareUserVehicleNumInternal(const void* key1, const void* key2) 
{
    return strcmp((const char*)key1, (const char*)key2);
}

// Parking: Compare parking_id (int*) with Parking* data
int compareParkingId(const void* key, const void* data) 
{
    return (*(const int*)key) - ((const Parking*)data)->parking_id;
}

// Parking: Compare parking_id (int*) with internal key (int*)
int compareParkingIdInternal(const void* key1, const void* key2) 
{
    return (*(const int*)key1) - (*(const int*)key2);
}


// Get key value from User data
void* getUserKey(const void* data) 
{
    return (void*)((const User*)data)->vehicle_num;
}

// Get key value from Parking data
void* getParkingKey(const void* data) 
{
    return (void*)&(((const Parking*)data)->parking_id);
}


// Create a copy of a User key
void* copyUserKey(const void* key)
{
    const char* original_key = (const char*)key;
    size_t key_len = strlen(original_key);
    char* new_key = (char*)malloc(key_len + 1);

    if (!new_key)
    {
        perror("Failed to allocate memory for user key copy");
        exit(EXIT_FAILURE);
    }

    strcpy(new_key, original_key);

    return (void*)new_key;
}

// Create a copy of a Parking key
void* copyParkingKey(const void* key) 
{
    int* new_key = (int*)malloc(sizeof(int));

    if (!new_key) 
    {
        perror("Failed to copy parking key");
        exit(EXIT_FAILURE);
    }

    *new_key = *(const int*)key;

    return (void*)new_key;
}

// Free a copied User key
void freeUserKey(void* key) 
{
    free(key);
}

// Free a copied Parking key
void freeParkingKey(void* key) 
{
    free(key);
}


// Print Functions
void printUser(const void* a) 
{
    const User* user = (const User*)a;
    printf("  Vehicle: %s (Owner: %s, Status: %s, Slot: %d, NOP: %d, TAP: %f)\n", user->vehicle_num, user->owner_name, user->status == PARKED ? "Parked" : "Not Parked", user->status == PARKED ? user->parking_space_id : -1,
                                                                            user->number_of_parkings, user->total_parking_amt);
}

void printParking(const void* a) 
{
    const Parking* p = (const Parking*)a;
    printf("  Parking ID: %d (Status: %s, Revenue: %.2f, Occupancies: %d)\n", p->parking_id, p->parking_space_status == OCCUPIED ? "Occupied" : "Vacant", p->revenue, p->occupancies);
}

// File Print Functions
void printUserInFile(const void* a, FILE* file) 
{
    const User* current = (const User*)a;

    fprintf(file, "\n%s,%s,%s,%s,%s,%s,%d,%d,%d,%.2f,%.2f,%.2f,%.2f,%d",
            current->vehicle_num, current->owner_name,
            current->arrival_date, current->arrival_time,
            current->departure_date, current->departure_time,
            current->parking_space_id, current->number_of_parkings,
            current->membership, current->spent_time,
            current->total_spent_time, current->parking_amt,
            current->total_parking_amt, (int)current->status);
}

void printParkingInFile(const void* a, FILE* file) 
{
    const Parking* current = (const Parking*)a;
    fprintf(file, "\n%d,%d,%.2f,%d", current->parking_id, current->parking_space_status, current->revenue, current->occupancies);

}



// B+ Tree Functions
GenericBPlusTreeNode* createGenericBPlusTreeNode(bool is_leaf) 
{
    GenericBPlusTreeNode* node = (GenericBPlusTreeNode*)malloc(sizeof(GenericBPlusTreeNode));

    if (!node) 
    {
        perror("Memory allocation failed for B+ Tree Node");
        exit(EXIT_FAILURE);
    }

    node->num_keys = 0;
    node->is_leaf = is_leaf;
    node->parent = NULL;
    node->next_leaf = NULL; // Important for B+ Tree leaves
    for (int i = 0; i < MAXKEYS; i++) node->keys[i] = NULL;
    for (int i = 0; i < MAXCHILDREN; i++) node->children[i] = NULL;

    return node;
}

// Find the leaf node where a key should exist or be inserted
GenericBPlusTreeNode* findLeaf(GenericBPlusTreeNode* root, const void* key, CompareInternalKeyFunc cmp_internal) 
{
    if (root == NULL)
    {
        return NULL;
    }
    
    GenericBPlusTreeNode* current = root;

    while (!current->is_leaf) 
    {
        int i = 0;
        // Find the first key greater than the search key
        while (i < current->num_keys && cmp_internal(key, current->keys[i]) >= 0) 
        {
            i++;
        }

        // Follow the appropriate child pointer
        current = current->children[i];
    }

    return current;
}

// Search for data associated with a key
void* Search_BPlus(GenericBPlusTreeNode* root, const void* key, CompareDataFunc cmp_data, CompareInternalKeyFunc cmp_internal) 
{
    if (root == NULL)
    {
        return NULL;
    }

    GenericBPlusTreeNode* leaf = findLeaf(root, key, cmp_internal);
    if (!leaf) return NULL;

    // Linear search within the leaf node
    for (int i = 0; i < leaf->num_keys; i++) 
    {
        if (cmp_data(key, leaf->keys[i]) == 0) 
        {
            return leaf->keys[i]; // Return pointer to the actual data
        }
    }

    return NULL; // Key not found in the leaf
}


// B+ Tree Insertion
// Helper to insert data pointer into a leaf node
status_code insertIntoLeaf(GenericBPlusTreeNode* leaf, void* dataPtr, GetKeyFromDataFunc getKey, CompareDataFunc cmp_data) 
{
    if (!leaf || !leaf->is_leaf || leaf->num_keys >= MAXKEYS) 
    {
        return FAILURE; // Should not happen if called correctly
    }

    void* key = getKey(dataPtr);
    int i = leaf->num_keys - 1;

    // Find position and shift existing keys
    while (i >= 0 && cmp_data(key, leaf->keys[i]) < 0) 
    {
        leaf->keys[i + 1] = leaf->keys[i];
        i--;
    }

    leaf->keys[i + 1] = dataPtr;
    leaf->num_keys = leaf->num_keys + 1;

    return SUCCESS;
}

// Helper to insert a key and child pointer into an internal node
status_code insertIntoParent(GenericBPlusTreeNode** rootRef, GenericBPlusTreeNode* left, void* key_copy, GenericBPlusTreeNode* right, CopyKeyFunc copyKey, FreeKeyFunc freeKey, CompareInternalKeyFunc cmp_internal) 
{
    GenericBPlusTreeNode* parent = left->parent;

    // Case 1: No parent (left was the root)
    if (parent == NULL) 
    {
        GenericBPlusTreeNode* new_root = createGenericBPlusTreeNode(false); // New root is internal
        if (!new_root) { freeKey(key_copy); return FAILURE; }

        new_root->keys[0] = key_copy; // key_copy is owned by new_root now
        new_root->children[0] = left;
        new_root->children[1] = right;
        new_root->num_keys = 1;
        left->parent = new_root;
        right->parent = new_root;
        *rootRef = new_root;

        return SUCCESS;
    }

    // Case 2: Parent has space
    if (parent->num_keys < MAXKEYS) 
    {
        int i = parent->num_keys - 1;
        // Find insertion point for key_copy
        while (i >= 0 && cmp_internal(key_copy, parent->keys[i]) < 0) 
        {
            parent->keys[i + 1] = parent->keys[i];
            parent->children[i + 2] = parent->children[i + 1];
            i--;
        }

        parent->keys[i + 1] = key_copy; // key_copy owned by parent now
        parent->children[i + 2] = right;
        parent->num_keys++;
        right->parent = parent; // Ensure right child's parent is updated

        return SUCCESS;
    } 
    else 
    {
        // Case 3: Parent is full - Split the parent (internal node)
        GenericBPlusTreeNode* new_internal = createGenericBPlusTreeNode(false);
        if (!new_internal) 
        { 
            freeKey(key_copy); 
            return FAILURE; 
        }

        // Temporary storage for keys and children
        void* temp_keys[MAXKEYS + 1];
        GenericBPlusTreeNode* temp_children[MAXCHILDREN + 1];

        int i = 0;
        // Copy existing keys/children before insertion point
        while (i < MAXKEYS && cmp_internal(key_copy, parent->keys[i]) > 0) 
        {
            temp_keys[i] = parent->keys[i];
            temp_children[i] = parent->children[i];
            i++;
        }

        temp_children[i] = parent->children[i]; // Copy child pointer at insertion point

        // Insert the new key and right child
        temp_keys[i] = key_copy;
        temp_children[i + 1] = right;

        // Copy remaining keys/children
        while (i < MAXKEYS) 
        {
            temp_keys[i + 1] = parent->keys[i];
            temp_children[i + 2] = parent->children[i + 1];
            i++;
        }

        // Distribute keys/children between old and new internal nodes
        // The median key is pushed up, not stored in the split nodes
        int split_point_key_index = MAXKEYS / 2; // Index of key to push up
        void* key_to_push_up = temp_keys[split_point_key_index]; // This key goes to the parent's parent

        parent->num_keys = split_point_key_index; // Keys before median stay
        new_internal->num_keys = MAXKEYS - split_point_key_index; // Keys after median go to new node

        // Copy keys/children to old parent (left side)
        for (i = 0; i < parent->num_keys; i++) 
        {
            parent->keys[i] = temp_keys[i];
            parent->children[i] = temp_children[i];
            
            if(parent->children[i]) parent->children[i]->parent = parent; // Update child's parent
        }


        parent->children[parent->num_keys] = temp_children[parent->num_keys]; // Last child pointer for old parent
        if(parent->children[parent->num_keys])
        {
            parent->children[parent->num_keys]->parent = parent;
        } 


         // Clear remaining pointers in old parent
        for(i = parent->num_keys; i < MAXKEYS; i++) parent->keys[i] = NULL;
        for(i = parent->num_keys + 1; i < MAXCHILDREN; i++) parent->children[i] = NULL;


        // Copy keys/children to new internal node (right side)
        // Note: key at split_point_key_index was pushed up, so start copying keys *after* it
        for (i = 0; i < new_internal->num_keys; i++) 
        {
            new_internal->keys[i] = temp_keys[split_point_key_index + 1 + i];
            new_internal->children[i] = temp_children[split_point_key_index + 1 + i];
            if(new_internal->children[i]) new_internal->children[i]->parent = new_internal; // Update child's parent
        }

        new_internal->children[new_internal->num_keys] = temp_children[MAXKEYS + 1]; // Last child pointer for new node

        if(new_internal->children[new_internal->num_keys])
        {
            new_internal->children[new_internal->num_keys]->parent = new_internal;
        }

         // Clear remaining pointers in new internal node
        for(i = new_internal->num_keys; i < MAXKEYS; i++) new_internal->keys[i] = NULL;
        for(i = new_internal->num_keys + 1; i < MAXCHILDREN; i++) new_internal->children[i] = NULL;


        new_internal->parent = parent->parent;

        // Recursively insert the pushed-up key into the parent's parent
        return insertIntoParent(rootRef, parent, key_to_push_up, new_internal, copyKey, freeKey, cmp_internal);
    }
}

// Main Insertion Function
status_code Insert_BPlus(GenericBPlusTreeNode** rootRef, void* dataPtr, CompareDataFunc cmp_data, CompareInternalKeyFunc cmp_internal, GetKeyFromDataFunc getKey, CopyKeyFunc copyKey, FreeKeyFunc freeKey)
{
    status_code sc = SUCCESS;

    // Handle Empty Tree
    if (*rootRef == NULL) 
    {
        *rootRef = createGenericBPlusTreeNode(true); // Root is initially a leaf
        (*rootRef)->keys[0] = dataPtr;
        (*rootRef)->num_keys = 1;

        sc = SUCCESS;
    }
    else
    {
        void* key = getKey(dataPtr); // Get the key from the data object

        // Find the target leaf node
        GenericBPlusTreeNode* leaf = findLeaf(*rootRef, key, cmp_internal);

        // Check for duplicates in leaf before inserting
        for (int i = 0; i < leaf->num_keys; i++) 
        {
            if (cmp_data(key, leaf->keys[i]) == 0) 
            {
                fprintf(stderr, "Error: Duplicate key insertion attempted.\n");
                sc = FAILURE;
            }
        }

        if(sc == SUCCESS)
        {
            // Check if leaf has space
            if (leaf->num_keys < MAXKEYS) 
            {
                return insertIntoLeaf(leaf, dataPtr, getKey, cmp_data);
            } 
            else 
            {
                // Leaf is full - Split the leaf
                GenericBPlusTreeNode* new_leaf = createGenericBPlusTreeNode(true);
                if (!new_leaf) return FAILURE;

                // Temporary storage for keys + new key
                void* temp_keys[MAXKEYS + 1];
                int i = 0;

                while (i < MAXKEYS && cmp_data(key, leaf->keys[i]) > 0) 
                {
                    temp_keys[i] = leaf->keys[i];
                    i++;
                }

                temp_keys[i] = dataPtr; // Insert the new data pointer
                while (i < MAXKEYS) 
                {
                    temp_keys[i + 1] = leaf->keys[i];
                    i++;
                }


                // Distribute keys between old and new leaf
                int split_point = (MAXKEYS + 1 + 1) / 2; // Ceil((MAXKEYS+1)/2)
                leaf->num_keys = split_point;
                new_leaf->num_keys = (MAXKEYS + 1) - split_point;

                for (i = 0; i < leaf->num_keys; i++) 
                {
                    leaf->keys[i] = temp_keys[i];
                }
                 // Clear remaining pointers in old leaf (optional but good practice)
                for (i = leaf->num_keys; i < MAXKEYS; i++) 
                {
                    leaf->keys[i] = NULL;
                }

                for (i = 0; i < new_leaf->num_keys; i++) 
                {
                    new_leaf->keys[i] = temp_keys[split_point + i];
                }

                // Clear remaining pointers in new leaf
                for (i = new_leaf->num_keys; i < MAXKEYS; i++) 
                {
                    new_leaf->keys[i] = NULL;
                }

                // Update parent pointers and leaf links
                new_leaf->parent = leaf->parent;
                new_leaf->next_leaf = leaf->next_leaf;
                leaf->next_leaf = new_leaf;

                // Get the key to copy up (first key of the new leaf)
                void* key_to_copy_up = getKey(new_leaf->keys[0]);
                void* key_copy = copyKey(key_to_copy_up); // Create a copy for the internal node

                // Insert the copied key into the parent
                return insertIntoParent(rootRef, leaf, key_copy, new_leaf, copyKey, freeKey, cmp_internal);
            }
        }
    }

    return sc;
}



// B+ Tree Traversal (Leaf Nodes)
void traverseLeaves(GenericBPlusTreeNode* root, PrintFunc print) 
{
    if (!root) 
    {
        printf("Tree is empty.\n");
        return;
    }

    // Find the leftmost leaf node
    GenericBPlusTreeNode* current = root;
    while (current && !current->is_leaf) 
    {
        current = current->children[0];
    }

    if (!current) 
    {
        printf("No leaf nodes found (tree might be corrupted or empty internal).\n");
        return;
    }


    // Traverse the linked list of leaves
    while (current != NULL) 
    {
        for (int i = 0; i < current->num_keys; i++) 
        {
            if (current->keys[i]) // Check if pointer is valid
            { 
                print(current->keys[i]);
            } 
            else 
            {
                printf("NULL data pointer found at index %d\n", i);
            }

        }
        printf("\n");
        current = current->next_leaf;
    }

    printf("-- End of Leaf Traversal --\n");
}


void traverseLeavesForFile(GenericBPlusTreeNode* root, PrintFuncFile print, FILE* file) 
{
    if (!root || !file) return;

    GenericBPlusTreeNode* current = root;

    while (current && !current->is_leaf) 
    {
        current = current->children[0];
    }
     
    if (!current) return; // Empty tree or no leaves

    while (current != NULL) 
    {
        for (int i = 0; i < current->num_keys; i++) 
        {

            if (current->keys[i]) 
            {
                print(current->keys[i], file);
            }
        }
        current = current->next_leaf;
    }
}


// Search User
User* SearchUser_BPlus(GenericBPlusTreeNode* userRoot, const char* vehicle_num) 
{
    return (User*)Search_BPlus(userRoot, vehicle_num, compareUserVehicleNum, compareUserVehicleNumInternal);
}

// Search Parking
Parking* SearchParking_BPlus(GenericBPlusTreeNode* parkingRoot, int parking_id) 
{
    return (Parking*)Search_BPlus(parkingRoot, &parking_id, compareParkingId, compareParkingIdInternal);
}


Parking* Find_Free_Slot(GenericBPlusTreeNode* parkingRoot, int min_id, int max_id, CompareInternalKeyFunc cmp_internal) 
{
    if (parkingRoot == NULL)
    {
        return NULL;
    }

    // Find the leaf node where min_id would reside or start
    GenericBPlusTreeNode* leaf = findLeaf(parkingRoot, &min_id, cmp_internal);

    GenericBPlusTreeNode* current_leaf = leaf;

    // Search starting from this leaf, potentially moving to next leaves
    while (current_leaf) 
    {
        for (int i = 0; i < current_leaf->num_keys; i++) 
        {
            Parking* p = (Parking*)current_leaf->keys[i];

            if (p->parking_id >= min_id && p->parking_id <= max_id) 
            {
                if (p->parking_space_status == VACANT) 
                {
                    return p; // Found a suitable vacant slot
                }
            }
        }

        current_leaf = current_leaf->next_leaf;
    }

    return NULL; // No vacant slot found in the range
}

bool Assign_Parking_ID(GenericBPlusTreeNode* parkingRoot, int min_id, int max_id, int* assigned_parking_id) 
{
    bool status = true;

    Parking* freeParkingSlot = Find_Free_Slot(parkingRoot, min_id, max_id, compareParkingIdInternal);

    if (freeParkingSlot == NULL) 
    {
        status = false;
    } 
    else 
    {
        *assigned_parking_id = freeParkingSlot->parking_id;
        freeParkingSlot->parking_space_status = OCCUPIED;
        freeParkingSlot->occupancies = freeParkingSlot->occupancies + 1;

        status = true;
    }

    return status;
}

bool Allocation_Policy(GenericBPlusTreeNode* parkingRoot, User* userNode, int* parking_id) 
{
    bool status = false;
    int min_id = -1;
    int max_id = 50;

    // Determine search range based on membership
    if (userNode->membership == 2)  // Gold
    { 
        min_id = 1; 
    }
    else if (userNode->membership == 1) // Premium
    { 
        min_id = 11;
    } 
    else 
    {
        min_id = 21;
    }

    status = Assign_Parking_ID(parkingRoot, min_id, max_id, parking_id);

    if (status) 
    {
        userNode->parking_space_id = *parking_id; // Update user struct immediately
    }

    return status;
}

bool Insert_Update(GenericBPlusTreeNode** parkingRootRef, GenericBPlusTreeNode** userRootRef, const char* vehicle_num, const char* owner_name, const char* arrival_date, const char* arrival_time)
{
    User* userFound = SearchUser_BPlus(*userRootRef, vehicle_num);
    bool status = true;
    
    if (userFound != NULL) 
    {
        if (userFound->status == PARKED) 
        {
            printf("Error: Vehicle %s is already parked.\n", vehicle_num);
            return false;
        }

        int parkingId = -1;
        bool allocation_success = Allocation_Policy(*parkingRootRef, userFound, &parkingId);

        if (!allocation_success) 
        {
            printf("No suitable parking space available for your membership level.\n");
            return false;
        } 
        else 
        {
            // Update existing user record
            strcpy(userFound->arrival_date, arrival_date);
            strcpy(userFound->arrival_time, arrival_time);
            strcpy(userFound->departure_date, "-");
            strcpy(userFound->departure_time, "-");
            userFound->status = PARKED;
            userFound->parking_space_id = parkingId;
            userFound->number_of_parkings++;
            userFound->parking_amt = 0;
            userFound->spent_time = 0;
            printf("Vehicle %s assigned to parking ID %d.\n", vehicle_num, parkingId);

            status = true;
        }
    } 
    else 
    {
        Parking* freeParkingSlot = Find_Free_Slot(*parkingRootRef, 21, 50, compareParkingIdInternal);

        if (freeParkingSlot == NULL) 
        {
            printf("Sorry, %s, no suitable parking space available for new users at the moment.\n", owner_name);
            return false;
        } 
        else 
        {
            // Create the full user object *with* the assigned parking ID
            User* newUser = createUser(vehicle_num, owner_name, arrival_date, arrival_time, freeParkingSlot->parking_id);

            // Insert the new user into the B+ Tree
            status_code insert_status = Insert_BPlus(userRootRef, newUser, compareUserVehicleNum, compareUserVehicleNumInternal, getUserKey, copyUserKey, freeUserKey);

            if (insert_status == SUCCESS) 
            {
                printf("Vehicle %s assigned to parking ID %d and added to database.\n", vehicle_num, freeParkingSlot->parking_id);
                freeParkingSlot->occupancies = freeParkingSlot->occupancies + 1;
                freeParkingSlot->parking_space_status = OCCUPIED;
                status = true;
            } 
            else 
            {
                freeUser(newUser); // Free the user object we created
                status = false;
            }
        }
    }

    return status;
}



void Time_spent(User* user) 
{
    struct tm arrival_tm = {0}, departure_tm = {0};
    time_t arrival, departure;
    double seconds_diff;

    sscanf(user->arrival_date, "%d/%d/%d", &arrival_tm.tm_mday, &arrival_tm.tm_mon, &arrival_tm.tm_year);
    sscanf(user->arrival_time, "%d:%d", &arrival_tm.tm_hour, &arrival_tm.tm_min);
    arrival_tm.tm_year -= 1900; 
    arrival_tm.tm_mon -= 1;

    // Parse departure date and time
    sscanf(user->departure_date, "%d/%d/%d", &departure_tm.tm_mday, &departure_tm.tm_mon, &departure_tm.tm_year);
    sscanf(user->departure_time, "%d:%d", &departure_tm.tm_hour, &departure_tm.tm_min);
    departure_tm.tm_year -= 1900;
    departure_tm.tm_mon -= 1; 

    arrival = mktime(&arrival_tm);
    departure = mktime(&departure_tm);

    seconds_diff = difftime(departure, arrival);

    float hours = (float)(seconds_diff / 3600.0);

    user->spent_time = hours;
    user->total_spent_time += hours; 

    printf("Time spent by %s: %.2f hours (Total: %.2f hours)\n", user->vehicle_num, user->spent_time, user->total_spent_time);
}

void Membership(User* user) 
{
    int old_membership = user->membership;

    if (user->total_spent_time >= 200) 
    {
        user->membership = 2; // Gold
    } 
    else if (user->total_spent_time >= 100) 
    {
        user->membership = 1; // Premium
    } 
    else 
    {
        user->membership = 0; // Standard
    }
}

void Payment(Parking* parking, User* user) 
{
    double integral;
    double fractional = modf((double)user->spent_time, &integral);

    int buffer = (int)(user->spent_time - fractional);
    int parking_amt = 0;

    if(fractional > 0.2)
    {
        buffer = buffer + 1;
    }

    if(user->spent_time >= 3.0)
    {
        parking_amt = 100 + 50 * ((int)(buffer - 3));

        if(user->membership != 0)
        {
            parking_amt = (parking_amt * 9) / 10;
        }
    }
    else
    {
        int parking_amt = 100;
        if(user->membership != 0)
        {
            parking_amt = (parking_amt * 9) / 10;
        }
    }

    user->parking_amt = parking_amt;
    user->total_parking_amt += parking_amt;
    parking->revenue += parking_amt;
}

bool Exit_Vehicle_BPlus(GenericBPlusTreeNode** parkingRootRef, GenericBPlusTreeNode** userRootRef, const char* vehicle_num, const char* departure_date, const char* departure_time)
{
    User* userFound = SearchUser_BPlus(*userRootRef, vehicle_num);

    if (!userFound) 
    {
        printf("Error: Vehicle %s not found in database.\n", vehicle_num);
        return false;
    }

    if (userFound->status == NOTPARKED) 
    {
        printf("Error: Vehicle %s is not currently parked.\n", vehicle_num);
        return false;
    }

    // Vehicle found and is parked
    int parkingId = userFound->parking_space_id;
    Parking* parkingFound = SearchParking_BPlus(*parkingRootRef, parkingId);

    // Update User Record
    strcpy(userFound->departure_date, departure_date);
    strcpy(userFound->departure_time, departure_time);
    userFound->status = NOTPARKED;

    Time_spent(userFound);
    Membership(userFound);

    Payment(parkingFound, userFound);
    parkingFound->parking_space_status = VACANT;

    userFound->parking_space_id = -1;

    printf("Vehicle %s has exited Parking Slot %d.\n", vehicle_num, parkingId);
    return true;
}

void PrintOneEntry_BPlus(GenericBPlusTreeNode* userRoot, const char* vehicle_num) 
{
    User* userFound = SearchUser_BPlus(userRoot, vehicle_num);

    if(userFound) 
    {
        printf("\n--- Details for Vehicle: %s ---\n", userFound->vehicle_num);
        printf("Owner name: %s\n", userFound->owner_name);
        printf("Membership: %d (%s)\n", userFound->membership, userFound->membership == 2 ? "Gold" : (userFound->membership == 1 ? "Premium" : "Standard"));
        printf("Status: %s\n", userFound->status == PARKED ? "Parked" : "Not Parked");

        if(userFound->status == PARKED)
        {
            printf("Current Parking space id: %d\n", userFound->parking_space_id);
            printf("Current Arrival Date: %s\n", userFound->arrival_date);
            printf("Current Arrival time: %s\n", userFound->arrival_time);
            printf("Current Time Spent (so far): Calculation requires exit.\n");
            printf("Current Parking amount due: Calculation requires exit.\n");
        }
        else 
        {
            printf("Last Arrival Date: %s\n", userFound->arrival_date);
            printf("Last Arrival time: %s\n", userFound->arrival_time);
            printf("Last Departure Date: %s\n", userFound->departure_date);
            printf("Last Departure time: %s\n", userFound->departure_time);
            printf("Last Time Spent: %.2f hours\n", userFound->spent_time);
            printf("Last Parking amount paid: %.2f\n", userFound->parking_amt);
        }

        printf("Total Time Spent: %.2f hours\n", userFound->total_spent_time);
        printf("Total Parking amount paid: %.2f\n", userFound->total_parking_amt);
        printf("Number of parkings done: %d\n", userFound->number_of_parkings);
        printf("---------------------------------\n");

    } 
    else 
    {
        printf("User with vehicle number '%s' does not exist in Database!\n", vehicle_num);
    }
}


// Read User Database
GenericBPlusTreeNode* READ_DATABASE_BPlus(const char* filename) 
{
    GenericBPlusTreeNode* userRoot = NULL;
    FILE* file = fopen(filename, "r");

    if (!file) 
    {
        perror("Unable to open user file for reading");
        return NULL;
    }

    char line[512];

    if (fgets(line, sizeof(line), file) == NULL) 
    {
        fclose(file);
        return NULL;
    }

    int line_num = 1;
    while (fgets(line, sizeof(line), file) != NULL) 
    {
        line_num++;

        User* newUser = (User*)malloc(sizeof(User));
        
        if (!newUser) 
        {
            perror("Unable to allocate memory for new user during read");
            fclose(file);
            return userRoot;
        }

        sscanf(line, "%19[^,],%49[^,],%10[^,],%5[^,],%10[^,],%5[^,],%d,%d,%d,%f,%f,%f,%f,%d",
            newUser->vehicle_num,
            newUser->owner_name,
            newUser->arrival_date,
            newUser->arrival_time,
            newUser->departure_date,
            newUser->departure_time,
            &newUser->parking_space_id,
            &newUser->number_of_parkings,
            &newUser->membership,
            &newUser->spent_time,
            &newUser->total_spent_time,
            &newUser->parking_amt,
            &newUser->total_parking_amt,
            (int*)&newUser->status);

        status_code status = Insert_BPlus(&userRoot, newUser, compareUserVehicleNum, compareUserVehicleNumInternal, getUserKey, copyUserKey, freeUserKey);
        
        if (status != SUCCESS) 
        {
            fprintf(stderr, "Failed to insert user record from line %d: %s\n", line_num, line);
            freeUser(newUser);
        }
    }

    fclose(file);
    printf("Read user records successfully.\n");

    return userRoot;
}

// Write User Database
void WRITE_DATABASE_BPlus(const char* filename, GenericBPlusTreeNode* userRoot) 
{
    FILE* file = fopen(filename, "w");

    if (!file) 
    {
        perror("Unable to open user file for writing");
        return;
    }

    fprintf(file, "Vehicle_Number,Owner_Name,Arrival_Date,Arrival_Time,Departure_Date,Departure_Time,Parking_Space_ID,Number_of_Parkings,Membership,Spent_Time,Total_Spent_Time,Parking_Amt,Total_Parking_Amt,Status");

    // Write user data by traversing leaves
    traverseLeavesForFile(userRoot, printUserInFile, file);

    fclose(file);

    printf("User database written successfully.\n");
}

// Read Parking Database
GenericBPlusTreeNode* READ_PARKING_BPlus(const char* filename) 
{
    GenericBPlusTreeNode* parkingRoot = NULL;
    FILE* file = fopen(filename, "r");
    bool file_existed = (file != NULL);

    if (!file) 
    {
        perror("Error, No parking database exists!");
        exit(EXIT_FAILURE);
    } 
    else 
    {
        char line[256];
        if (fgets(line, sizeof(line), file) == NULL)
        {
            // File exists but is empty, treat as initialization case?
            fclose(file);
        }

        int line_num = 1;
        while (fgets(line, sizeof(line), file) != NULL) 
        {
            line_num++;

            Parking* newParking = (Parking*)malloc(sizeof(Parking));

                if (!newParking) 
                {
                    perror("Unable to allocate memory for new user");
                    fclose(file);
                    return NULL;
                }

                // Parse the line
                sscanf(line,"%d, %d, %f, %d", &newParking->parking_id, &newParking->parking_space_status, &newParking->revenue, &newParking->occupancies);

                status_code insert_status = Insert_BPlus(&parkingRoot, newParking, compareParkingId, compareParkingIdInternal, getParkingKey, copyParkingKey, freeParkingKey);
                if(insert_status == FAILURE)
                {
                    fprintf(stderr, "Failed to insert parking record from line: %s\n", line);
                    free(newParking);
                    fclose(file);
                    exit(EXIT_FAILURE);
                }
        }

        fclose(file);

        printf("Read parking data successfully from %s.\n", filename);
    }

    return parkingRoot;
}

// Write Parking Database
void WRITE_PARKING_BPlus(const char* filename, GenericBPlusTreeNode* parkingRoot) 
{
    FILE* file = fopen(filename, "w");

    if (!file) 
    {
        perror("Unable to open parking file for writing");
        return;
    }

    fprintf(file, "Parking_ID,Status,Revenue,Occupancies");

    traverseLeavesForFile(parkingRoot, printParkingInFile, file);

    fclose(file);
    printf("Parking database written successfully.\n");
}

// B+ Tree Destruction
void destroyGenericBPlusTreeNode(GenericBPlusTreeNode* node, FreeDataFunc freeData, FreeKeyFunc freeKey) 
{
    if (!node) return;

    if (!node->is_leaf) 
    {
        // Recursively destroy children
        for (int i = 0; i < node->num_keys + 1; ++i) 
        {
            destroyGenericBPlusTreeNode(node->children[i], freeData, freeKey);
        }

        // Free internal keys (copies)
        for (int i = 0; i < node->num_keys; ++i) 
        {
            if (node->keys[i]) 
            {
                freeKey(node->keys[i]);
            }
        }
    } 
    else 
    {
        // Free actual data pointers in leaf
        if (freeData) 
        {
            for (int i = 0; i < node->num_keys; ++i) 
            {
                 if (node->keys[i]) 
                 {
                    freeData(node->keys[i]);
                }
            }
        }
    }

    free(node);
}

void Destroy_BPlus_Tree(GenericBPlusTreeNode** rootRef, FreeDataFunc freeData, FreeKeyFunc freeKey) 
{
    if (rootRef && *rootRef) 
    {
        destroyGenericBPlusTreeNode(*rootRef, freeData, freeKey);
        *rootRef = NULL;
    }
}



ListNode* createSimpleNode(void* data) 
{
    ListNode* newNode = (ListNode*)malloc(sizeof(ListNode));

    if (!newNode) 
    {
        perror("Failed to allocate ListNode");
        return NULL;
    }

    newNode->data = data;
    newNode->next = NULL;
    return newNode;
}

void freeSimpleList(ListNode* head) 
{
    ListNode* current = head;
    ListNode* nextNode;

    while (current != NULL) 
    {
        nextNode = current->next;
        free(current);
        current = nextNode;
    }
}

// Extracts all data pointers from primary tree leaves into a linked list
ListNode* extractToList(GenericBPlusTreeNode* primaryRoot) 
{
    if (!primaryRoot) return NULL;

    ListNode* head = NULL;
    ListNode* tail = NULL;
    GenericBPlusTreeNode* current_primary_leaf = primaryRoot;

    // Find the leftmost leaf
    while (current_primary_leaf && !current_primary_leaf->is_leaf) 
    {
        current_primary_leaf = current_primary_leaf->children[0];
    }

    if (!current_primary_leaf) return NULL;

    while (current_primary_leaf != NULL) 
    {
        for (int i = 0; i < current_primary_leaf->num_keys; i++) 
        {
            void* original_data_ptr = current_primary_leaf->keys[i];
            if (!original_data_ptr) continue;

            ListNode* newNode = createSimpleNode(original_data_ptr);

            if (!newNode) 
            {
                fprintf(stderr, "Failed to create list node during extraction. Aborting.\n");
                return NULL;
            }

            if (tail == NULL) 
            {
                head = tail = newNode;
            } 
            else 
            {
                tail->next = newNode;
                tail = newNode;
            }
        }

        current_primary_leaf = current_primary_leaf->next_leaf;
    }

    return head;
}



// Comparison function pointer type for list sorting
typedef int (*ListCompareFunc)(const void* dataA, const void* dataB);

int compareUsersByNumParkings_list(const void *a, const void *b) 
{
    User *userA = (User *)a; 
    User *userB = (User *)b;

    if (userA->number_of_parkings != userB->number_of_parkings)
    {
        return userA->number_of_parkings - userB->number_of_parkings;
    }
    else
    {
        return strcmp(userA->vehicle_num, userB->vehicle_num);
    }
}

int compareUsersByParkingAmt_list(const void *a, const void *b) 
{
    User *userA = (User *)a; 
    User *userB = (User *)b;

    if (userA->total_parking_amt != userB->total_parking_amt)
    {
        return (userA->total_parking_amt > userB->total_parking_amt) ? 1 : -1;
    }
    else
    {
        return strcmp(userA->vehicle_num, userB->vehicle_num);
    }    
}

int compareParkingByOccupancy_list(const void *a, const void *b) 
{
    Parking *pA = (Parking *)a;
    Parking *pB = (Parking *)b;

    if (pA->occupancies != pB->occupancies)
    {
        return pB->occupancies - pA->occupancies; // Descending
    }
    else
    {
        return pA->parking_id - pB->parking_id;
    }   
    
}

int compareParkingByRevenue_list(const void *a, const void *b) 
{
    Parking *pA = (Parking *)a; 
    Parking *pB = (Parking *)b;

    if (pA->revenue != pB->revenue)
    {
        return (pB->revenue > pA->revenue) ? 1 : -1; // Descending
    }
    else
    {
        return pA->parking_id - pB->parking_id;
    }
    
}

ListNode* getMiddle(ListNode* head) 
{
    if (head == NULL) return head;
    ListNode* slow = head;
    ListNode* fast = head->next;

    while (fast != NULL && fast->next != NULL) 
    {
        slow = slow->next;
        fast = fast->next->next;
    }

    return slow;
}

ListNode* mergeSortedLists(ListNode* a, ListNode* b, ListCompareFunc cmp) 
{
    if (a == NULL) return b;
    if (b == NULL) return a;

    ListNode* result = NULL;
    if (cmp(a->data, b->data) <= 0) 
    {
        result = a;
        result->next = mergeSortedLists(a->next, b, cmp);
    } 
    else 
    {
        result = b;
        result->next = mergeSortedLists(a, b->next, cmp);
    }

    return result;
}

void mergeSortList(ListNode** headRef, ListCompareFunc cmp) 
{
    ListNode* head = *headRef;
    ListNode* a;
    ListNode* b;

    // Base case: 0 or 1 element
    if (head == NULL || head->next == NULL) 
    {
        return;
    }

    // Split list into 'a' and 'b' halves
    ListNode* middle = getMiddle(head);
    a = head;
    b = middle->next;
    middle->next = NULL; // Terminate the first half

    // Recursively sort the two halves
    mergeSortList(&a, cmp);
    mergeSortList(&b, cmp);

    // Merge the sorted halves
    *headRef = mergeSortedLists(a, b, cmp);
}

void printSimpleList(ListNode* head, PrintFunc printData, const char* listName) 
{
    if (!printData) 
    {
        printf("Error: Cannot print list '%s'. No data printing function provided.\n", listName);
        return;
    }

    printf("\n--- Printing Linked List: %s ---\n", listName);

    if (head == NULL) 
    {
        printf("List is empty.\n");
        printf("--- End of List: %s ---\n\n", listName);
        return;
    }

    ListNode* current = head;
    int count = 1;

    while (current != NULL) 
    {
        printf("Node %d: \n", count++);
        if (current->data != NULL) 
        {
            printData(current->data);
        } 
        else 
        {
            printf("Node contains NULL data pointer\n");
        }

        current = current->next;
    }

    printf("--- End of List ---\n\n");
}


int compareUserAmtVsBoundary(const User* userData, const float boundaryVal) 
{
    if (!userData || !boundaryVal) 
    {
        return -1;
    }
    
    const User* user = userData;
    const float boundaryAmount = boundaryVal;

    if (user->total_parking_amt < boundaryAmount) 
    {
        return -1; // User amount is less than the boundary
    } 
    else 
    {
        return 1; // User amount is greater than the boundary
    }
}

void printSimpleListRange(ListNode* head,const float min_val,const float max_val, PrintFunc printData) 
{
    printf("\n--- Printing List ---\n");

    if (head == NULL) 
    {
        printf("List is empty. No items to check in range.\n");
        printf("--- End of List ---\n\n");
        return;
    }

    ListNode* current = head;
    int count = 0; // Count of items printed within the range
    bool first_printed = true;

    while (current != NULL) 
    {
        if (current->data != NULL) 
        {
            // Check if data is within the range [min_val, max_val]
            bool greater_equal_min = (compareUserAmtVsBoundary(current->data, min_val) >= 0);
            bool less_equal_max    = (compareUserAmtVsBoundary(current->data, max_val) <= 0);

            if (greater_equal_min && less_equal_max) 
            {
                // Data is in range, print it
                if (first_printed) 
                {
                    printf("Items found within the specified range:\n");
                    first_printed = false;
                }

                printData(current->data);
                count++;
            }
        }
        current = current->next;
    }

    if (count == 0) 
    {
        printf("No items found within the specified range.\n");
    }

    printf("--- End of List (%d items printed) ---\n\n", count);
}


void UsersByNumParkings_ListTree(GenericBPlusTreeNode* userRootPrimary) 
{
    if (!userRootPrimary) 
    { 
        printf("Primary user tree is empty.\n");
        return; 
    }

    ListNode* userList = extractToList(userRootPrimary);
    if(!userList) 
    { 
        printf("Failed to extract user data to list.\n"); 
        return; 
    }

    mergeSortList(&userList, compareUsersByNumParkings_list);

    printf("\n>>> Printing Sorted List <<<\n");
    printSimpleList(userList, printUser, "Sorted User List (by Num Parkings)");
    
    freeSimpleList(userList);
    userList = NULL;

}

void UsersByParkingAmountRange_ListTree(GenericBPlusTreeNode* userRootPrimary) 
{
    if (!userRootPrimary) 
    { 
        printf("Primary user tree is empty.\n");
        return; 
    }

    ListNode* userList = extractToList(userRootPrimary);
    if(!userList) 
    { 
        printf("Failed to extract user data to list.\n"); 
        return; 
    }

    float min_amount, max_amount;
    printf("Enter minimum parking amount: ");
    scanf("%f", &min_amount);

    printf("\nEnter maximum parking amount: ");
    scanf("%f", &max_amount);

    if (min_amount > max_amount)
    { 
        printf("Min > Max invalid.\n"); 
        return; 
    }

    mergeSortList(&userList, compareUsersByParkingAmt_list);

    printSimpleListRange(userList, min_amount, max_amount, printUser);

    freeSimpleList(userList);
    userList = NULL;

}

void ParkingByOccupancy_ListTree(GenericBPlusTreeNode* parkingRootPrimary)
{
    if (!parkingRootPrimary) 
    { 
        printf("Primary parking tree is empty.\n");
        return; 
    }

    ListNode* parkingList = extractToList(parkingRootPrimary);
    if(!parkingList) 
    { 
        printf("Failed to extract parking data to list.\n"); 
        return; 
    }

    mergeSortList(&parkingList, compareParkingByOccupancy_list);

    printf("\n>>> Printing Sorted List <<<\n");
    printSimpleList(parkingList, printParking, "Sorted Parking List (by Occupancy)");

    freeSimpleList(parkingList);
    parkingList = NULL;
}

void ParkingByRevenue_ListTree(GenericBPlusTreeNode* parkingRootPrimary)
{    
    if (!parkingRootPrimary) 
    { 
        printf("Primary parking tree is empty.\n");
        return; 
    }

    ListNode* parkingList = extractToList(parkingRootPrimary);
    if(!parkingList) 
    { 
        printf("Failed to extract parking data to list.\n"); 
        return; 
    }

    mergeSortList(&parkingList, compareParkingByRevenue_list);


    printf("\n>>> Printing Sorted List <<<\n");
    printSimpleList(parkingList, printParking, "Sorted Parking List (by Revenue)");

    freeSimpleList(parkingList);
    parkingList = NULL;
}

int main() 
{
    char vehicle_num[20];
    char owner_name[50];
    char arrival_date[11];
    char arrival_time[6];
    char departure_date[11];
    char departure_time[6];

    bool status = true;
    int temp;

    // Initialize Parking B+ Tree
    GenericBPlusTreeNode* parkingRoot = READ_PARKING_BPlus("sample_parking.csv");

    // Initialize User B+ Tree
    GenericBPlusTreeNode* userRoot = READ_DATABASE_BPlus("sample_user.csv");

    printf("\n--- Initial B+ Tree States ---\n");
    printf("User Tree Leaves:\n");
    traverseLeaves(userRoot, printUser);
    printf("\nParking Tree Leaves:\n");
    traverseLeaves(parkingRoot, printParking);
    printf("-----------------------------\n\n");

    int choice;
    do 
    {
        printf("\n--- Parking Management Menu ---\n");
        printf("[1] Enter Vehicle\n");
        printf("[2] Exit Vehicle\n");
        printf("[3] View Vehicle Details\n");
        printf("[4] Sort Vehicle Users\n");
        printf("[5] Sort Parking Spaces\n");
        printf("[0] Exit and Save\n");
        printf("-------------------------------\n");
        printf("[*] Enter choice: ");

        scanf("%d", &choice);
        switch (choice) 
        {
            case 1:
                printf("Vehicle number:\n");
                scanf("%20s", vehicle_num);

                printf("Owner Name:\n");
                scanf("%50s", owner_name);

                printf("Enter Arrival Date (DD/MM/YYYY):\n");
                scanf("%11s", arrival_date);

                printf("Arrival time (HH:MM):\n");
                scanf("%6s", arrival_time);

                status = Insert_Update(&parkingRoot, &userRoot, vehicle_num, owner_name, arrival_date, arrival_time);

                if(status) 
                {
                    printf("Vehicle entry processed successfully.\n");
                } 
                else 
                {
                    printf("Vehicle entry failed.\n");
                }
                
                break;

            case 2:
                printf("Vehicle number:\n");
                scanf("%20s", vehicle_num);

                printf("Enter Departure Date (DD/MM/YYYY):\n");
                scanf("%11s", departure_date);

                printf("Departure time (HH:MM):\n");
                scanf("%6s", departure_time);

                status = Exit_Vehicle_BPlus(&parkingRoot, &userRoot, vehicle_num, departure_date, departure_time);

                if(status) 
                {
                    printf("Vehicle exit processed successfully.\n");
                } 
                else
                {
                    printf("Vehicle exit failed.\n");
                }

                break;

            case 3:
                printf("Enter Vehicle Number\n");
                scanf("%20s", vehicle_num);
                printf("\n");

                PrintOneEntry_BPlus(userRoot, vehicle_num);
                break;

             case 4:
                printf("Enter [0] to Sort the Vehicle List based on Number of Parkings\n");
                printf("Enter [1] to Sort the Vehicle List based on Parking Amount Paid\n");
                printf("\n[*] Option: ");
                scanf("%d", &temp);

                if(!temp)
                {
                    UsersByNumParkings_ListTree(userRoot);
                }
                else
                {
                    UsersByParkingAmountRange_ListTree(userRoot);
                }
                break;

             case 5:
                printf("Enter [0] to Sort the Parking List based on Occupancies\n");
                printf("Enter [1] to Sort the Parking List based on Revenue\n");
                printf("\n[*] Option: ");
                scanf("%d", &temp);

                if(!temp)
                {
                    ParkingByOccupancy_ListTree(parkingRoot);
                }
                else
                {
                    ParkingByRevenue_ListTree(parkingRoot);
                }
                break;

            case 0:
                printf("Exiting and saving data...\n");
                break;

            default:
                printf("Invalid choice. Please try again.\n");
                break;
        }
    } while (choice != 0);

    // Save data to files before exiting
    WRITE_DATABASE_BPlus("sample_user.csv", userRoot);
    WRITE_PARKING_BPlus("sample_parking.csv", parkingRoot);

    // Clean up memory
    printf("Cleaning up resources...\n");
    Destroy_BPlus_Tree(&userRoot, freeUser, freeUserKey);
    Destroy_BPlus_Tree(&parkingRoot, freeParking, freeParkingKey);


    printf("Thank You!\n");
    return 0;
}