#include <stdio.h>
#include <stdlib.h>

typedef struct node
{
	int num;
	struct node* next;
}node;

void display(node* head)
{
	node *temp = NULL;
	temp = head;
	while(temp!= NULL)
	{
		printf("%d-->", temp->num);
		temp = temp->next;
	}
}
void reverse(node **head)
{
	node *current = NULL;
	node *previous = NULL;
	node *next = NULL;
	current = *head;
	printf("\nThe reversed linked list ll be:\n");
	while(current != NULL)
	{
		next = current->next;
		current->next = previous;
		previous = current;
		current = next;
	}
	*head = previous;
	display(*head);
}
int main()
{
	node *head = NULL;
	node *temp = NULL;
	int data;
	for(int i=0; i<5; i++)
	{
		temp = head;
		if(i== 0)
		{
			head = (struct node *) malloc (sizeof(struct node));
			temp = head;
		}
		else
		{
			while(temp->next != NULL)
			{
				temp = temp->next;
			}
			temp->next = (struct node *) malloc (sizeof(struct node));
			temp = temp->next;
		}
		scanf("%d", &data);
		temp->num = data;
		temp->next = NULL;
	}
	display(head);
	reverse(&head);
	return 0;
}

