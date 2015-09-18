
#ifndef LIST_H_
#define LIST_H_
//#include <mutex>
//
//typedef struct node_s  node_t;
//
//struct node_s {
//	node_t  *prev;
//	node_t  *next;
//};


#define list_init(q)            \
	(q)->prev = q;              \
	(q)->next = q


#define list_empty(h)           \
	(h == (h)->prev)


#define list_insert_head(h, x)   \
    (x)->next = (h)->next;       \
    (x)->next->prev = x;         \
    (x)->prev = h;               \
    (h)->next = x


#define list_insert_after   list_insert_head


#define list_insert_tail(h, x)   \
    (x)->prev = (h)->prev;       \
    (x)->prev->next = x;         \
    (x)->next = h;               \
    (h)->prev = x


#define list_head(h)             \
    (h)->next


#define list_last(h)             \
    (h)->prev


#define list_sentinel(h)         \
    (h)


#define list_next(q)             \
    (q)->next


#define list_prev(q)             \
    (q)->prev


#define list_remove(x)             \
    (x)->next->prev = (x)->prev;   \
    (x)->prev->next = (x)->next




#define list_split(h, q, n)        \
    (n)->prev = (h)->prev;         \
    (n)->prev->next = n;           \
    (n)->next = q;                 \
    (h)->prev = (q)->prev;         \
    (h)->prev->next = h;           \
    (q)->prev = n;


#define list_add(h, n)             \
    (h)->prev->next = (n)->next;   \
    (n)->next->prev = (h)->prev;   \
    (h)->prev = (n)->prev;         \
    (h)->prev->next = h;


#define list_data(q, type, link)   \
    (type *) ((unsigned char *) q - offsetof(type, link))


template <class T>
class LinkedList
{
public:
	LinkedList()
	{
		list_ = (T*)malloc(sizeof(T));
		list_init(list_);
	}

	~LinkedList()
	{
		while (!list_empty(list_))
		{
			T* t = list_->next;
			list_remove(list_->next);
			delete t;
		}
		free(list_);
	}

	bool empty()
	{
		return list_empty(list_);
	}

	void  push_back(T* t)
	{
		list_insert_tail(list_, t);
	}

	void  remove(T* t)
	{
		list_remove(t);
	}

	void insert(T* pos, T* t)
	{
		list_add(pos, t);
	}

	T* next(T* t)
	{
		if (list_next(t) == list_ || t == list_)
		{
			return NULL;
		}
		return list_next(t);
	}

	T* end() { return list_; }
	//取出一个
	T* front()
	{
		if (list_empty(list_))
		{
			return NULL;
		}

		return list_->next;
	}

	//取并移除一个
	T* pop_front()
	{
		if (list_empty(list_))
		{
			return NULL;
		}

		T* t = list_->next;
		list_remove(t);
		return t;
	}

	void push_front(T* t)
	{
		list_insert_head(list_, t);
	}


private:
	T* list_;
};



//example:
//
//struct Book{  
//	node_t qLink;  
//	int type;    
//};
//
//void testList()
//{
//	auto t = clock();
//	node_t* head = (node_t*) malloc(sizeof(node_t)); 
//	list_init(head);  
//
//	for (int i=gCounts; i>0; --i)
//	{
//		Book* b = (Book*) malloc(sizeof(Book));
//		b->type = i;
//		list_insert_tail(head, &(b->qLink));
//	}
//
//	for (int i=0; i<gCounts; ++i)
//	{
//		node_t* t = head->next;
//		list_remove(t);
//		list_insert_tail(head, t);
//	}
//	cout<<"testListQ: "<<clock() - t<<endl;
//
//	node_t* i = head->next;
//	node_t* e = list_last(head);
//	while (i != e)
//	{
//		Book * b =(Book*)i;
//		cout<<b->type<<endl;
//		i = list_next(i);
//	}
//
//}

#endif /* LIST_H */
