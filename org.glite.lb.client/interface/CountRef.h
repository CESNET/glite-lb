#ifndef __EDG_WORKLOAD_LOGGING_CLIENT_COUNTREF_HPP__
#define __EDG_WORKLOAD_LOGGING_CLIENT_COUNTREF_HPP__

#define EWL_BEGIN_NAMESPACE namespace edg { namespace workload { namespace logging { namespace client {
#define EWL_END_NAMESPACE } } } }

EWL_BEGIN_NAMESPACE;

template<typename T>
class CountRef {
public:
	CountRef(void *);
//	CountRef(void *,void (*)(void *));

	void use(void);
	void release(void);

	void	*ptr;
private:
	int	count;
//	void	(*destroy)(void *);
};

template <typename T>
CountRef<T>::CountRef(void *p)
{
	ptr = p;
	count = 1;
}

template <typename T>
void CountRef<T>::release(void)
{
	if (--count == 0) {
		T::destroyFlesh(ptr);
		delete this;
	}
}

template <typename T>
void CountRef<T>::use(void)
{
	count++;
}

EWL_END_NAMESPACE;

#endif
