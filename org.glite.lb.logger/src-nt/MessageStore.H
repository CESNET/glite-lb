/*
Copyright (c) Members of the EGEE Collaboration. 2004-2010.
See http://www.eu-egee.org/partners/ for details on the copyright holders.

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

#ifndef _MESSAGE_STORE_H_
#define _MESSAGE_STORE_H_

#include <pthread.h>

/** Permanent storage for messages and their states.
 */

class MessageStore {
public:

	/** Base class for everything that can be stored here.
	 */
	class Storable {
	public:
		/** Get size needed for object storage.
		 */
		virtual int getStorageSize() const = 0;
		
		/** Save state of object into binary data.
		 */
		virtual int save(void* data, int len) const = 0;

		/** Load state of object from binary data.
		 */
		virtual int load(void* data, int len) = 0;

		virtual ~Storable() {}
	};


	/** Class that uniquely identifies stored content.
	 */
	class ID: public Storable {
	public:
		/** Default constructor.
		 *
		 * Creates new unique ID.
		 */
		ID();

		/** Copy constructor.
		 */
		ID(const ID& src);

		/** Destructor.
		 */
		~ID() {};

		/** Assignment operator.
		 */
		ID& operator=(const ID& src);

		/** Return the string suitable for printing.
		 */
		std::string toString() const;

		/** Comparison operator
		 */
		int operator==(const ID& second);

		/** Get size needed for storage (from Storable).
		 */
		virtual int getStorageSize() const;

		/** Save ID (from Storable)
		 */
		virtual int save(void* data, int len) const;

		/** Load ID (from Storable)
		 */
		virtual int load(void* data, int len);

	protected:
		unsigned long long getID() {return id;}

	private:
		static pthread_mutex_t counterLock;
		static unsigned counter;
		unsigned long long id;
	};
};

#endif
