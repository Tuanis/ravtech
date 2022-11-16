#include <stdio.h>
#include <string.h>
#include <stddef.h> /* for NULL */
#include <ctype.h>  /* for isspace() */
#include "list.h"

/* a file of comma separated values (CSV)
 * Each line represents one customer's info with values
 * firstname,secondname,id,phone,date,debt
 */
#define DATAFILE "makolet.csv"

ELEMENT *insert_customer(ELEMENT *, ELEMENT *);
void show_customers(void);
void accept_queries(void);
void show_instructions(void);
void trim_end(char *);

static ELEMENT *head = NULL;
static FILE    *fp;

int main()
{
    char    data[100];
    ELEMENT *element;

    /* No need to open for read/append, because first we read the entire file */
    fp = fopen(DATAFILE, "r+"); // read/write

    // First Element
    do {
        if (fscanf(fp,"%s",data) == 1){
            head = create_element(data);
        }
    } while (head == NULL); // can happen if data of first line doesn't validate

    if (head != NULL) { // can happen that all the lines of the CSV didn't validate
        // Other elements
        while (fscanf(fp,"%s",data) == 1) {
            element  = create_element(data);
            if (element != NULL) { // can happen if data of current line doesn't validate
                head = insert_customer(head,element);
            }
        }
    }

    show_customers();
    accept_queries();

    fclose(fp);

    /* Apparently it is not _mandatory_ to free just before program exit,
     * and some claim it isn't even a good idea.
     * Yet some say it is a good habit...
     */
    destroy_list(head);

    return 0;
}

ELEMENT *insert_customer(ELEMENT *head, ELEMENT *element) {
    ELEMENT *dup;
    int new_day, new_month, new_year,
        old_day, old_month, old_year;
    double new_debt, old_debt, total;
    char new_date[MAXDATE+1], old_date[MAXDATE+1];

    dup = find_element(head,element->customer.id);
    if (dup != NULL) { // found someone with the same id!
        /* I used to do
         * if (compare_names(dup->customer.firstname,element->customer.firstname) == 0) {}
         * with a function that uses tolower(), or that calls strcmpi().
         * But I understood from the instructions that names are to be printed in lowercase,
         * so might as well convert to lower before inserting!
         */
        if (strcmp(dup->customer.firstname,element->customer.firstname) != 0) {
            printf("Dup ID (%s) with different first names (%s vs %s)\n",element->customer.id,dup->customer.firstname,element->customer.firstname);
            return head; // no changes made
        }
        // else if (compare_names(dup->customer.secondname,element->customer.secondname) == 0) {
        else if (strcmp(dup->customer.secondname,element->customer.secondname) != 0) {
            printf("Dup ID (%s) with different last names (%s vs %s)\n",element->customer.id,dup->customer.secondname,element->customer.secondname);
            return head; // no changes made
        }
        else {
            // compare dates
            sscanf(dup->customer.date,"%d/%d/%d",&old_day,&old_month,&old_year);
            sscanf(element->customer.date,"%d/%d/%d",&new_day,&new_month,&new_year);
            sprintf(new_date,"%d%02d%02d",new_year,new_month,new_day);
            sprintf(old_date,"%d%02d%02d",old_year,old_month,old_day);
            if (strcmp(new_date,old_date) >= 0){ // need to update info
                strcpy(dup->customer.phone, element->customer.phone);
                strcpy(dup->customer.date,  element->customer.date);
            }
            /* I guess that the debt can have a decimal point? Not clear from instructions... */
            old_debt = strtod(dup->customer.debt,NULL);
            new_debt = strtod(element->customer.debt,NULL);
            total = old_debt + new_debt;
            if (total == (int) total) // if there are no decimals, show as integer
                sprintf(dup->customer.debt,"%d",(int) total);
            else
                sprintf(dup->customer.debt,"%6.2lf",total);

            // Now that the debt changed, need to insert the element in the correct position: disconnect and re-insert
            head = disconnect_element(head, dup);
            return insert_element(head, dup);
        }
    }
    else{
        return insert_element(head, element);
    }
}

void show_customers(void) {
    ELEMENT *p = head;
    show_headers();
    while (p != NULL){
        show_customer(&(p->customer));
        p = p->next;
    }
}

void accept_queries(void) {
    char query[200], copy[200], *command, *rest, *part;
    int keep_going = 1;

    printf("\nReady for your query:\n");

    while (keep_going){
        printf("\n--> ");
        gets(query);

        /* There may be more than one "select" command in the query, separated by commas
         * For example: select debt<-20.5, select date < 2/3/2022
         * But, a single "set" command also has comma separated parts!
         * For example: set first name=Moshe, second name=Berdichevsky, id=123456789, phone=0544123456, date=3/4/2022, dept=-300
         */

        rest = query;
        strcpy(copy,query); // keep the original
        command = strtok_r(query," ",&rest);
        if (strcmpi(command, "quit") == 0) {
            printf("Good bye!\n");
            keep_going = 0;
        }
        else if (strcmpi(command, "select") == 0) {
            char field[12], op[3], value[20], *other;
            SEARCH search = {0};
            int no_error = 1;

            part = strtok_r(copy,",\n",&other); // the original subquery, it includes the word "select"
            while (part != NULL) {
                field[0] = op[0] = value[0] = '\0'; // a more efficient 'clear' than strcpy with ""

                sscanf(strlwr(part)," select %[^!=<>] %[!=<>] %s",field,op,value);
                // If user enters a space between "first name" and "=" (for ex.) the string field will have a space at the end... trim!
                trim_end(field);
                
                if (strcmpi(field,"first name") == 0){
                    strcpy(search.firstname,value);
                    if (strcmp(op,"=") == 0)
                        search.firstop = equal;
                    else if (strcmp(op,"!=") == 0)
                        search.firstop = unequal;
                    else if (strcmp(op,"<") == 0)
                        search.firstop = less;
                    else if (strcmp(op,">") == 0)
                        search.firstop = more;
                    else { // it really never gets here due to the scanf format...
                        printf("Incorrect operator: %s", op);
                        no_error = 0;
                    }
                }
                else if (strcmpi(field,"second name") == 0){
                    strcpy(search.secondname,value);
                    if (strcmp(op,"=") == 0)
                        search.secondop = equal;
                    else if (strcmp(op,"!=") == 0)
                        search.secondop = unequal;
                    else if (strcmp(op,"<") == 0)
                        search.secondop = less;
                    else if (strcmp(op,">") == 0)
                        search.secondop = more;
                    else {
                        printf("Incorrect operator: %s", op);
                        no_error = 0;
                    }
                }
                else if (strcmp(field,"id") == 0){
                    strcpy(search.id,value);
                    if (strcmp(op,"=") == 0)
                        search.idop = equal;
                    else if (strcmp(op,"!=") == 0)
                        search.idop = unequal;
                    else if (strcmp(op,"<") == 0)
                        search.idop = less;
                    else if (strcmp(op,">") == 0)
                        search.idop = more;
                    else {
                        printf("Incorrect operator: %s", op);
                        no_error = 0;
                    }
                }
                else if (strcmp(field,"phone") == 0){
                    strcpy(search.phone,value);
                    if (strcmp(op,"=") == 0)
                        search.phoneop = equal;
                    else if (strcmp(op,"!=") == 0)
                        search.phoneop = unequal;
                    else if (strcmp(op,"<") == 0)
                        search.phoneop = less;
                    else if (strcmp(op,">") == 0)
                        search.phoneop = more;
                    else {
                        printf("Incorrect operator: %s", op);
                        no_error = 0;
                    }
                }
                else if (strcmp(field,"date") == 0){
                    strcpy(search.date,value);
                    if (strcmp(op,"=") == 0)
                        search.dateop = equal;
                    else if (strcmp(op,"!=") == 0)
                        search.dateop = unequal;
                    else if (strcmp(op,"<") == 0)
                        search.dateop = less;
                    else if (strcmp(op,">") == 0)
                        search.dateop = more;
                    else {
                        printf("Incorrect operator: %s", op);
                        no_error = 0;
                    }
                }
                else if (strcmpi(field,"debt") == 0){
                    strcpy(search.debt,value);
                    if (strcmp(op,"=") == 0)
                        search.debtop = equal;
                    else if (strcmp(op,"!=") == 0)
                        search.debtop = unequal;
                    else if (strcmp(op,"<") == 0)
                        search.debtop = less;
                    else if (strcmp(op,">") == 0)
                        search.debtop = more;
                    else {
                        printf("Incorrect operator: %s\n", op);
                        no_error = 0;
                    }
                }
                else {
                    printf("Incorrect field: %s\n", field);
                    no_error = 0;
                }
                part = strtok_r(NULL,",\n",&other);
            }
            if (no_error)
                search_customers(head,search);
        }
        else if (strcmpi(command, "set") == 0) {
            ELEMENT *element;
            char    firstname[MAXNAME+1], secondname[MAXNAME+1], id[25], phone[25], date[25], debt[25],
                    data[200];

            sscanf(rest,"%*[^=] = %[^,], %*[^=] = %[^,], %*[^=] = %[^,], %*[^=] = %[^,], %*[^=] = %[^,], %*[^=] = %s",
                firstname,secondname,id,phone,date,debt);

            sprintf(data,"%s,%s,%s,%s,%s,%s",firstname,secondname,id,phone,date,debt);
            printf("\nSetting: %s\n",data);
            if ((element = create_element(data)) != NULL) {
                fprintf(fp,"%s\n",data); // append to csv
                fflush(fp);
                head = insert_customer(head,element);
            }

            /* perhaps show how SET affected the data? */
            // show_customers();
        }
        else if (strcmpi(command, "print") == 0) {
            show_customers();
        }
        else
            show_instructions();
    }
}

void show_instructions(void) {
    printf ("\n%s\n\n%s\n%s\n%s\n%s\n%s\n",
        "You can choose one of the following commands:",
        "select - to show the customer data filtered on some field(s)",
        "set    - to either update an existing customer, or add a new one",
        "print  - to show all customer data as was shown at the beginning",
        "help   - to show these instructions again",
        "quit   - to exit");
}

void trim_end(char *str) {
    int i = strlen(str);

    while (i --> 0) { // the "goes down to" 'operator' is really a combination of decreasing (--) and greater-than (>)  :-)
        if (!isspace(str[i])) {
            str[i+1] = '\0';
            break;
        }
    }
}