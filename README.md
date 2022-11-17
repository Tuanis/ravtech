# Makolet

Manages entries in a shop's notebook, stored in a CSV file

Each line of the CSV contains information about a debt or payment from a certain customer.

Each line contains 6 items:

First Name, Second Name (Last Name), ID, Phone, Date, Debt/Payment

The program reads in the info, validates each field, and adds it to the database (stored as a linked list)

If the ID already exists, and names match, it will update the other info (phone, debt) as needed (based on the more recent date)

After all the data is in, the program prints the info in order of debt (descending)

It then shows a prompt so that the user can choose among the following actions:
* Print the list again
* Set - to enter a new line into the notebook file and into the database (with validation and update as above)
* Select - to query the database
* Help
* Quit

Query format is "Select field_name operator value"

Accepted operators are "=", "!=", "<", ">"

A query can contain more than one select command, separated by commas, to add conditions to the search
