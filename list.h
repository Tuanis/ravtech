#include <stdio.h>
#include <stddef.h> // for NULL
#include <string.h> // for strcpy
#include <stdlib.h> /* for malloc and free */

#define MAXNAME 12
#define MAXID 9
#define MAXPHONE 10
#define MAXDATE 10
#define MAXDEBT 10

typedef struct {
    char firstname[MAXNAME+1];
    char secondname[MAXNAME+1];
    char id[MAXID+1];
    char phone[MAXPHONE+1];
    char date[MAXDATE+1];
    char debt[MAXDEBT+1];
} CUSTOMER;

typedef enum op {none,equal,unequal,less,more} op;

typedef struct {
    char firstname[MAXNAME+1];
    op firstop;
    char secondname[MAXNAME+1];
    op secondop;
    char id[MAXID+1];
    op idop;
    char phone[MAXPHONE+1];
    op phoneop;
    char date[MAXDATE+1];
    op dateop;
    char debt[MAXDEBT+1];
    op debtop;
} SEARCH;

struct list { // singly linked list, to hold all the customer data sorted by debt DESC
    CUSTOMER    customer;
    struct list *next;
};

typedef struct list ELEMENT;

void show_headers(void) {
    printf("\n%-12s %-12s %-9s %-10s %-10s %-6s\n","First Name","Second Name","ID","Phone","Date","Debt");
    printf("%-12s %-12s %-9s %-10s %-10s %-6s\n","============","============","=========","==========","==========","======");
}
void show_customer(CUSTOMER *customer) {
    printf("%-12s %-12s %-9s %-10s %-10s %-6s\n",
           customer->firstname,
           customer->secondname,
           customer->id,
           customer->phone,
           customer->date,
           customer->debt);
}

/* I saw there is a strlwr() function, but seems to be deprecated?
 * If so, replace with this... (must #include <ctype.h>)
void lowercase(unsigned char *str) {
    while (*str) {
        *str = tolower(*str);
        str++;
    }
}
*/

ELEMENT *create_element(char *data){
    ELEMENT *element;
    char    firstname[MAXNAME+1], secondname[MAXNAME+1],
            // using big strings to allow for mistakes in the file, and still allow sscanf to work correctly
            id[50],phone[50],date[50],debt[50];
    int n, len, dd, mm, yyyy, conversions;

    /* Preferred using sscanf instead of strtok, because I know I need 6, and not less */
    /* A scan that succeeds will set the value of n, and data[n] should be the null character */
    if ((conversions = sscanf(data,"%[^,],%[^,],%[^,],%[^,],%[^,],%[^,]%n",firstname,secondname,id,phone,date,debt,&n) != 6) || (data[n] != '\0')){
        printf("Incorrect number of values (%d)! 6 are expected: %s\n",conversions,data);
        return NULL;
    }

	/* Do some data validation */

    if ((len = strlen(id)) != 9 || (int) strspn(id,"0123456789") < len) {
        printf("Invalid ID: %s\n",id);
        return NULL;
    }
    /*   - Phone must be 10 digits long and begin with 0 */
    if (phone[0] != '0' || (len = strlen(phone)) != 10 || (int) strspn(phone,"0123456789") < len) {
        printf("Invalid Phone: %s\n",phone);
        return NULL;
    }
    /*   - Date must have format dd/mm/yyyy */
    if ((sscanf(date,"%d/%d/%d%n",&dd,&mm,&yyyy,&n) != 3) || (date[n] != '\0') || !(dd >= 1 && dd <= 31 ) || !(mm >= 1 && mm <= 12)) {
        printf("Invalid Date: %s\n",date);
        return NULL;
    }
    /*   - Debt must be a number (negative, zero or positive) */
    if ( (int) strspn(debt,"0123456789-.") < strlen(debt)) {
        printf("Invalid Debt: %s\n",debt);
        return NULL;
    }

    /* If we got this far, we can create the element! */
    element = malloc(sizeof(ELEMENT));

    /* Populate fields */
    strcpy(element->customer.firstname , strlwr(firstname));
    strcpy(element->customer.secondname, strlwr(secondname));
    strcpy(element->customer.id        , id);
    strcpy(element->customer.phone     , phone);
    strcpy(element->customer.date      , date);
    strcpy(element->customer.debt      , debt);

    element->next = NULL;
    return element;
}

ELEMENT *find_element(ELEMENT *head, char *id) {
    if (head == NULL || strcmp(head->customer.id, id) == 0)
        return head;
    else
        return find_element(head->next, id);
}

ELEMENT *insert_element(ELEMENT *head, ELEMENT *element) {
    ELEMENT *current;
    double currdebt, elemdebt;

    /* Insert in order of debt (descending)
     * (Basically this is doing Insert-Sort once when reading the data file, and
     * then for every "SET" command.
     * This should be better than sorting before each PRINT command, even with a more efficient
     * sort algorithm)
     */
    sscanf(element->customer.debt,"%lf",&elemdebt);

    // If this is the first element of a new list
    if (head == NULL) {
        element->next = NULL;
        return element;
    }
    else {
        // See if insertion needs to happen before the head
        sscanf(head->customer.debt,"%lf",&currdebt);
        // Tried (strcmp(current->customer.debt,element->customer.debt) <= 0) -- but doesn't work for minus?
        if (currdebt <= elemdebt) {
            element->next = head;
            return element; // it became the new head
        }
        else {
            for (current = head ; current->next != NULL ; current = current->next) {
                sscanf(current->next->customer.debt,"%lf",&currdebt);
                if (currdebt <= elemdebt) {
                    // insert element _after_ current element
                    element->next = current->next;
                    current->next = element;
                    return head;
                }
            }
            // o/w insert at the end...
            current->next = element;
            return head;
        }
    }
}

ELEMENT *disconnect_element(ELEMENT *head, ELEMENT *element) {
    ELEMENT *prev; // the element before the one that gets disconnected

    if (element == head) { // it's the first element
        head = element->next; // the new head
        element->next = NULL; // in preparation for re-inserting
        return head;
    }
    else {
        for (prev = head; prev->next != element; prev = prev->next)
            ;
        prev->next = element->next;
        return head;
    }
    /* w/o freeing, since the idea is to re-insert in a diff place */
}

void search_customers(ELEMENT *head, SEARCH conditions) {
    int match, similarity, d1, m1, y1, d2, m2, y2;
    double value1, value2;
    char date1[MAXDATE+1],date2[MAXDATE+1];

    show_headers();
    for (ELEMENT *p = head; p != NULL; p = p->next) {
        match = 1;
        if (conditions.firstop != none) {
            similarity = strcmpi(p->customer.firstname, conditions.firstname);
            match *= ((conditions.firstop == less && similarity < 0) ||
                      (conditions.firstop == more && similarity > 0) ||
                      (conditions.firstop == equal && similarity == 0) ||
                      (conditions.firstop == unequal && similarity != 0));
        }
        if (conditions.secondop != none) {
            similarity = strcmpi(p->customer.secondname, conditions.secondname);
            match *= ((conditions.secondop == less && similarity < 0) ||
                      (conditions.secondop == more && similarity > 0) ||
                      (conditions.secondop == equal && similarity == 0) ||
                      (conditions.secondop == unequal && similarity != 0));
        }
        if (conditions.idop != none) {
            value1 = strtod(p->customer.id,NULL);
            value2 = strtod(conditions.id,NULL);
            match *= ((conditions.idop == less && value1 < value2) ||
                      (conditions.idop == more && value1 > value2) ||
                      (conditions.idop == equal && value1 == value2) ||
                      (conditions.idop == unequal && value1 != value2));
        }
        if (conditions.phoneop != none) {
            value1 = strtod(p->customer.phone,NULL);
            value2 = strtod(conditions.phone,NULL);
            match *= ((conditions.phoneop == less && value1 < value2) ||
                      (conditions.phoneop == more && value1 > value2) ||
                      (conditions.phoneop == equal && value1 == value2) ||
                      (conditions.phoneop == unequal && value1 != value2));
        }
        if (conditions.dateop != none) {
            sscanf(p->customer.date,"%d/%d/%d",&d1,&m1,&y1);
            sscanf(conditions.date,"%d/%d/%d",&d2,&m2,&y2);
            sprintf(date1,"%d%02d%02d",y1,m1,d1);
            sprintf(date2,"%d%02d%02d",y2,m2,d2);
            similarity = strcmp(date1,date2);
            match *= ((conditions.dateop == less && similarity < 0) ||
                      (conditions.dateop == more && similarity > 0) ||
                      (conditions.dateop == equal && similarity == 0) ||
                      (conditions.dateop == unequal && similarity != 0));
        }
        if (conditions.debtop != none) {
            value1 = strtod(p->customer.debt,NULL);
            value2 = strtod(conditions.debt,NULL);
            match *= ((conditions.debtop == less && value1 < value2) ||
                      (conditions.debtop == more && value1 > value2) ||
                      (conditions.debtop == equal && value1 == value2) ||
                      (conditions.debtop == unequal && value1 != value2));
        }

        if (match) // none of the required conditions caused multiplication by 0
            show_customer(&(p->customer));
    }
}

void destroy_list(ELEMENT *head) {
    ELEMENT *p, *q;
    for (q = p = head ; p != NULL; ) {
        p = p->next;
        free(q);
        q = p;
    }
}