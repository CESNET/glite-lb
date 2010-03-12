/*
Copyright (c) Members of the EGEE Collaboration. 2004-2010.
See http://www.eu-egee.org/partners for details on the copyright holders.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

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
