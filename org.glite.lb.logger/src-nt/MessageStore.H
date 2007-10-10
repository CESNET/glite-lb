#ifndef _MESSAGE_STORE_H_
#define _MESSAGE_STORE_H_

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
		~ID();

		/** Assignment operator.
		 */
		ID& operator=(const ID& src);

		/** Return the string suitable for printing.
		 */
		std::string toString() const;

		/** Comparison operator
		 */
		int operator==();

		/** Get size needed for storage (from Storable).
		 */
		virtual int getStorageSize() const;

		/** Save ID (from Storable)
		 */
		virtual int save(void* &data, int len) const;

		/** Load ID (from Storable)
		 */
		virtual int load(void* data, int len);

	private:
	};
};

#endif
