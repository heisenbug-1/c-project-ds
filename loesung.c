#define _POSIX_C_SOURCE 200809L
#define _CRT_SECURE_NO_WARNINGS

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>

int splitInput (char *line, int sizeS); //for splitting the stdin in words and separators
int splitLine (char *substr, int size); //for splitting the dictionary into key<->value
void freeHashTable();
#define MODE_WB (1)				/* mode: read the dictionary */
#define MODE_IN (2)				/* mode: read from stdin */
#define COLLISION_PARAM (107)   /* hashset + 107 in case of collision */

unsigned long ht_size = 2;   	/* power of 2 for doubling the table size */
								/* unsigned int -> 2147483647, 32 Bit unsigned long 9223372036854775807, 64 Bit */

struct element //element in a hashtable
{
	char* key;
	char* value;
} *ht;
unsigned long element_count;  /* # of elements in  the hash table */

unsigned long hashFunction(const char *k)
{
	unsigned long int hashval = 0;
	int i = 0;

	/* Convert our string to an integer */
	while (hashval < ULONG_MAX && i < strlen(k))
	{
		hashval = hashval << 8;
		hashval += k[i];
		i++;
	}

	return hashval % ht_size;
}

int initHashtabelle(void) //initialize hash table and set key<-> value pairs to NULL
{
	unsigned long i;
	element_count = 0;
	ht = (struct element *)malloc(sizeof(*ht)*ht_size); //check if malloc failed

	if(ht == NULL)
	{
		fputs("Out of memory!", stderr);
		return 2;
	}

	for (i = 0; i < ht_size; i++)
	{
		ht[i].key = NULL;
		ht[i].value = NULL;
	}

	return 0;
}

char searchInHashTable(char *k, unsigned long *index) //search before insert to avoid duplicates
{
	/*found (1) == index of an element; 0 == position where can be inserted*/
	*index = hashFunction(k);
	unsigned long i = *index;
	int count = 0;
	do
	{
		if (NULL == ht[*index].key)
		{
			return 0; //can insert here
		}
		else if (k[0] == ht[*index].key[0] && 0 == strcmp(k, ht[*index].key))
		{
			return 1; 
		}

		*index = (*index + COLLISION_PARAM) % ht_size; //rehash
		count++;
	} while (*index != i);

    return 0;
}

int resizeHashTable(void) // resize to insert a new element
{
    unsigned long i, hashInd;
	struct element *current_table;
	current_table = ht;
	/* verdoppeln */
	ht_size <<= 1; 
	ht = (struct element *) malloc(sizeof(*ht)*ht_size);

	if(ht == NULL) //malloc checks
	{
		fputs("Out of memory!", stderr);
		return 2;
	}

	for (i = 0; i < ht_size; i++) //new hashtable, set key<->value to NULL
	{
		ht[i].key = NULL;
		ht[i].value = NULL;
	}

	/* shift the table */
	for (i = 0; i < (ht_size >> 1); i++) 
	{
		if (current_table[i].key != NULL)//only cope if the element is not empty
		{
			/* shift elements*/
			if (!searchInHashTable(current_table[i].key, &hashInd))
			{
				memcpy(&ht[hashInd], &current_table[i], sizeof(struct element));
			}
			/*else {
				fputs("cant's resize the hash table\n", stderr);

				  //free(current_table) freeHashTable()
				exit(5); 

			}*/
		}
	}
	/* delete the old table */
	free(current_table);
	return 0;
}

int insertHashTable(char *k, char *val)
{
	unsigned long i;
	if (!searchInHashTable(k, &i)) /* if key not found */
	{
		ht[i].key = strdup(k);
		ht[i].value = strdup(val);
		if (++element_count > ((unsigned long) ht_size * 0.9)) //if  the hash table 90% full
		{ 
			if(resizeHashTable() != 0)
			{
				return 1;
			}
		}
	}
	else
	{
		fputs("Duplicate found", stderr);
		return 2;
	}

	return 0;
}

void freeHashTable() //free key, value, table
{
	unsigned long i;
	for (i = 0; i < ht_size; i++)
	{
		if (ht[i].key != NULL) 
		{
			free(ht[i].key);
			free(ht[i].value);
		}
	}
	free(ht);
}

char* toLowercase(char *key) //to convert the stdin to lowercase letters
{
    char *low = strdup(key);
    unsigned long i = 0;
    while (low[i]) 
    {
        low[i] = (tolower(low[i]));
        i++;
    }

	return low;
}

int readDictLine(FILE *f, int mod)
{
    char c; //for reading a char
	char *substr = NULL; //save chars till you hit '/n' or EOF is this array
	int size = 0;
	int n = 0;

	while ((c = fgetc(f)))
	{
		size++; 

		if (substr == NULL)
		{
			substr = (char *)malloc(sizeof(char) * size); //
			if(substr == NULL)//if malloc fails
			{
				fputs("Out of memory!", stderr);
				return 2;
			}
		}
		else
		{
			char *temp = (char *)realloc(substr, sizeof(char) * size);

			if(temp == NULL) //if realloc fails
			{
				fputs("Out of memory!", stderr);
				free(substr);
				return 2;
			}

			substr = temp;
		}

		if ((c == '\n') && (mod == 1))
		{
			if (size == 1)
			{
				// empty line
				fputs("Empty line in wb file!", stderr);
				free(substr);
				return 2;
			}

			c = '\0'; //terminate the string
			n++;
		}

		substr[size - 1] = c;

		if (c == '\0')
		{
			if(size > 1 && splitLine(substr, size) != 0)
			{
				free(substr);
				fputs("zero character in file!", stderr);
				return 3;
			}

			size = 0;
		}

		else if (c == EOF)
		{
			if (mod == 1)
			{
				substr[size - 1] = '\0';
				n++;

				if(size > 1 && splitLine (substr, size) != 0)
				{
					free(substr);
					return 3;
				}

				size = 0;
			}
			else
			{
				substr[size-1] = '\0';

				if(splitInput (substr, size) != 0)
				{
					free(substr);
					return 4;
				}

			}
			break;
		}
		else if (mod == 1 && c != '\n' /*&& c == '\0'*/ && c != ':' && (c < 'a' || c > 'z'))
		{
			/* check if character is valid for wb file */
			/*if (c >= 'A' && c <= 'Z')
			{
				fputs("Upper case letter in wb file!", stderr);

			}*/

			fprintf(stderr, "Invalid character <%c> in wb file!", c);
			free(substr);
			return 5;
		}
		else if (mod == 2 && c != '\n' && c != '\0' && (c < 32 || c > 126))
		{
			/* check if character is valid for stdin */

			fprintf(stderr, "Invalid character <%c> in input!", c);
			free(substr);
			return 5;
		}
	}

	free(substr);
	return 0;
}

int splitLine (char *substr, int size) //
{
 	if(strchr(substr, ':') != strrchr(substr, ':')) // detect multiple :
	{
		fputs("Invalid pair!\nToo many colons!\n\n", stderr);
        return 1;
	}

	/* parse key */
	char* key = strtok(substr, ":");
	/* parse value */
	char* value = strtok(NULL, ":");

	/* triggers if line is empty or single colon */
	if(key == NULL) 
	{
		fputs("Invalid pair!\nKey missing!\n\n", stderr);
        return 1;
	}

	/* triggers for :abc or abc: */
	if(value == NULL)
	{
		fputs("Invalid pair!\nValue missing!\n\n",stderr);
        return 1;
	}
	
	if(insertHashTable(key, value) != 0)
	{
		return 1;
	}
	/* print values on success */

	return 0;
}

int splitInput (char *line, int sizeS)
{
	char *substr = NULL;
	char *substr2 = NULL;
	int sizeA = 0;
	int sizeN = 0;
	int k = 0;

	unsigned long i;

	while (line[k]) // line[k]!=0 oder line0[k]!='\0'
	{ 
		char c = line[k];

		if (isalpha(c)) 
		{
			sizeA++;
		}
		else
		{ 
			sizeN++;
		}

		if (substr == NULL)
		{
			substr = (char *)malloc(sizeof(char) * (sizeA + 1));

			if(substr == NULL)
			{
				fputs("Out of memory!", stderr);
				return 2;
			}
		}
		else
		{
			char *temp = (char *)realloc(substr, sizeof(char) * (sizeA + 1));

			if(temp == NULL)
			{
				fputs("Out of memory!", stderr);
				free(substr);
				return 2;
			}

			substr = temp;
		}

		if (substr2 == NULL)
		{
			substr2 = (char *)malloc(sizeof(char) * (sizeN + 1));

			if(substr2 == NULL)
			{
				fputs("Out of memory!", stderr);
				return 2;
			}
		}
		else
		{
			char *temp = (char *)realloc(substr2, sizeof(char) * (sizeN + 1));

			if(temp == NULL)
			{
				fputs("Out of memory!", stderr);
				free(substr2);
				return 2;
			}

			substr2 = temp;
		}

		if (isalpha(c)) 
		{
			if (sizeN) 
			{
				substr2[sizeN] = '\0';
				fputs(substr2, stdout);
				sizeN = 0;
				free(substr2);
				substr2 = NULL;
			}

			substr[sizeA - 1] = c;
		}
		else 
		{
			if (sizeA) 
			{
				substr[sizeA] = '\0';
				char *lowerCase = toLowercase(substr);

				if (!searchInHashTable(lowerCase, &i))
				 {
					putc('<', stdout);
					fputs(substr, stdout);
					putc('>', stdout);
				}
				else 
				{
					if(isupper(substr[0]))
					{							
						putc(ht[i].value[0] - 32, stdout);
						fputs(ht[i].value + 1, stdout);
					}
					else
					{							
						fputs(ht[i].value, stdout);
					}
				}

				free(lowerCase);
				sizeA = 0;
				free(substr);
				substr = NULL;
			}
			substr2[sizeN - 1] = c;
		}
		k++;
	}
	
	if (sizeA)
	{
		substr[sizeA] = '\0';

		char *lowerCase = toLowercase(substr);
		if (!searchInHashTable(lowerCase, &i))
		{
			
			putc('<', stdout); //if both valid but the word is not in the dictionary <>
			fputs(substr, stdout);
			putc('>', stdout);
		}
		else 
		{
			
			if(isupper(substr[0])) //if the word is uppercase
			{
				putc(ht[i].value[0] - 32, stdout);
				fputs(ht[i].value + 1, stdout);
			}
			else
			{					
				fputs(ht[i].value, stdout); //match
			}
		}

		free(lowerCase);
	}
	if (sizeN)
	{
		substr2[sizeN] = '\0';

		fputs(substr2, stdout);
	}

	free(substr);
	free(substr2);

	return 0;
}

int main(int argc, char *argv[])
{
	char *f_wb;
	FILE *file_wb, *file_in;

	if(initHashtabelle() != 0)
	{
		fputs("Failed to initalise hashtable!", stderr);
		return 2;
	}
	
    if(argc == 2) 
    {
	  	f_wb = argv[1];
	}
	else
	{
		fputs("No argument given!\n", stderr);
		freeHashTable();
		return 3;
	}

    file_wb = fopen(f_wb, "r");

    if(file_wb == NULL)
    {
    	fputs("Could not open given file!\n", stderr);
    	freeHashTable();
		fclose(file_wb);
    	return 3;
    }
	
	if(readDictLine(file_wb, MODE_WB) != 0)
	{
		freeHashTable();
		fclose(file_wb);
		return 2;
	}
	fclose(file_wb);
	
    file_in = stdin;

	if(readDictLine(file_in, MODE_IN) != 0)
	{
		freeHashTable();
		return 2;
	}
	
	freeHashTable();

	return 0;
}
