#ifndef GLITE_LB_PADSTRUCT_H
#define GLITE_LB_PADSTRUCT_H

#define glite_lb_padded_struct(_padded_name,_padded_size,_padded_content) \
	struct _padded_name##_to_pad__dont_use { _padded_content }; \
	struct _padded_name { \
		_padded_content \
		char _padding[_padded_size*sizeof(void *) - sizeof(struct _padded_name##_to_pad__dont_use)]; \
	};

#define glite_lb_padded_union(_padded_name,_padded_size,_padded_content) \
	union _padded_name##_to_pad__dont_use { _padded_content } ; \
	struct _padded_name##_to_check_pad__dont_use { char pad[_padded_size*sizeof(void *) - sizeof(union _padded_name##_to_pad__dont_use)]; }; \
	union _padded_name { _padded_content char _pad[_padded_size*sizeof(void *)]; };

#endif /* GLITE_LB_PADSTRUCT_H */
