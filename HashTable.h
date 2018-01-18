#include <string>
#include <utility>
#include <iostream>
#include <functional>
using namespace std;

template <class TYPE>
class LPTable:public Table<TYPE>{
	struct Record{
		//Data of the record.
		TYPE data_;
		//The records key.
		string key_;
		//Flag which indicates whether a record
		//has been marked as removed.
		//True = Removed
		bool remFlag_;
		Record(const string& key, const TYPE& data){
			key_=key;
			data_=data;
			remFlag_=false;
		}
	};
	//Hash function which accepts a string as its parameter.
	std::hash<std::string> hashFunction;

	//Pointer to a Record structure which would be implemented as a Record array.
	Record** records_;

	//Maximum of allowed records within the array of records.
	int max_;

	//The current size of the array of records.
	int size_;

	//Collision Key - used to keep track of collision point
	int colKey_;

	//Current Key - used to keep track of the current hashed key being used
	int currKey_;

	//Removal Key - used to keep track of the first dead/removed key found
	//							during a search by confirming if the remFlag_ within
	//							records has been changed to true.
	int remKey_;

	//Function which computes the Id from the given key by using a
	//hash funtion which is then modded to the size of the table.
	int getHashedId(const string& key) const;

	//Maximum space within the array of records
	int maxOpen_;

	//This value is a container for the incoming value from find()
	TYPE foundValue_;

	//This flag is set to true when a dead/removed record has
	//been traversed over.
	bool loopFlag_;
public:
	LPTable(int maxExpected,double percentOpen);
	LPTable(const LPTable& other);
	LPTable(LPTable&& other);
	virtual bool update(const string& key, const TYPE& value);
	virtual bool remove(const string& key);
	virtual bool find(const string& key, TYPE& value);
	virtual const LPTable& operator=(const LPTable& other);
	virtual const LPTable& operator=(LPTable&& other);
	virtual ~LPTable();
	virtual bool isEmpty() const {return size_==0;}
	virtual int numRecords() const {return size_;}
};

//This function's only argument is the key that is being
//hashed. It's purpose is to compute the Id from the given
//key by using a hash funtion which is then modded to the
//size of the table.
//This function will return an int which indicates the
//collision key and the hash key of the functions argument.
template <class TYPE>
int LPTable<TYPE>::getHashedId(const string& key)const{
	size_t hash=hashFunction(key);
	return hash%maxOpen_;
}
//This is the constructor for the LPTable class.
//It would construct a table and initialize an array of
//records based on the provided arguments:
//an int maxExpected and a double percentOpen.
template <class TYPE>
LPTable<TYPE>::LPTable(int maxExpected,double percentOpen): Table<TYPE>(){
	//Initializes maxOpen_ to an equation which provides
	//the amount of fields that should be created once taking
	//into account the wanted perent openess within the table.
	maxOpen_=maxExpected*(1+percentOpen);
	//Initializes the array of records to maxOpen_
	records_=new Record*[maxOpen_];
	//Initializes the max amount of allowed records within the table
	max_=maxExpected;
	//Sets the size to 0 to indiciate no records within the table
	size_=0;
	//Set keys and flags to default
	colKey_=-1;
	remKey_=-1;
	loopFlag_=false;
	currKey_=-1;
}
//This is the Copy Constructor which constructs a new table by copying another table
template <class TYPE>
LPTable<TYPE>::LPTable(const LPTable<TYPE>& other){
	//Sets the maximum size of the array
	maxOpen_=other.maxOpen_;
	//Initializes the array of records to maxOpen_
	records_=new Record*[maxOpen_];
	//Sets the max amount of allowed records
	max_=other.max_;
	//Sets the amount of active records that are being passed over from the other table
	size_=other.size_;
	//Setting currKey_ for while loop
	currKey_=0;
	//Dynamically adds the other table's records to this table
	while (currKey_<maxOpen_) {
		if(other.records_[currKey_]!=nullptr){
			string key=other.records_[currKey_]->key_;
			TYPE data=other.records_[currKey_]->data_;
			records_[currKey_]=new Record(key,data);
			records_[currKey_]->remFlag_=other.records_[currKey_]->remFlag_;
		}
		currKey_++;
	}
	//Set keys and flags to default
	colKey_=-1;
	remKey_=-1;
	loopFlag_=false;
	currKey_=-1;
}
//This is the Move Constructor which constructs a new table by moving records
//from another table to this one.
template <class TYPE>
LPTable<TYPE>::LPTable(LPTable<TYPE>&& other){
	//Sets currKey_ for while loop
	currKey_=0;
	//Sets the maximum size of the array
	maxOpen_=other.maxOpen_;
	//Initializes the array of records to maxOpen_
	records_=new Record*[maxOpen_];
	//Sets the max amount of allowed records
	max_=other.max_;
	//Sets the amount of active records that are being passed over from the other table
	size_=other.size_;
	//Dynamically adds the other table's records to this table
	//whilst dynamically de-allocating resources from the records
	//within the other table.
	while (currKey_<maxOpen_) {
		if(other.records_[currKey_]!=nullptr){
			string key=other.records_[currKey_]->key_;
			TYPE data=other.records_[currKey_]->data_;
			records_[currKey_]=new Record(key,data);
			records_[currKey_]->remFlag_=other.records_[currKey_]->remFlag_;
			delete other.records_[currKey_++];
		}
		currKey_++;
	}

	//Set keys and flags to default
	currKey_=-1;
	colKey_=-1;
	remKey_=-1;
	loopFlag_=false;
	//Deletes the array of records within the other table
	delete [] other.records_;
}
//This function takes two arguments: a string called key and a
//templated data type called value. The key is used to determine
//the collision key as well as to confirm if the key already exists
//within the table of records. The value will be used to either change
//an existing record with matching keys or to add a new record into the
//table with the key and value arguments.
//This function returns a boolean:
//True - if a new record has been created
//			 or if an existing record has been changed.
//False - if no update has occured
template <class TYPE>
bool LPTable<TYPE>::update(const string& key, const TYPE& value){
	//Search for the desird key within the table.
	//If a matching key has been found, the data within the record
	//that holds the key would be changed to the value argument.
	//Then return True.
	if(find(key,foundValue_)){
		//Change the current records data to the argument value
		records_[currKey_]->data_=value;
		//Set keys and flags to default
		remKey_=-1;
		currKey_=-1;
		loopFlag_=false;
		//Return True - update of record completed
		return true;
	}
	//If there is still space to add records to the table
	//and the current record is null, continue...
	else if(size_<max_ && records_[currKey_]==nullptr){
		//If the loopFlag_ is True - this indicates that a removed record
		//has been traversed over and set.
		//An insertion shall occur at the removed record and the remFlag_
		//shall be changed to false to indicate that this is a new record.
		if(loopFlag_){
			records_[remKey_]->key_=key;
			records_[remKey_]->data_=value;
			records_[remKey_]->remFlag_=false;
			//Increment the amount of records within the table
			size_++;
			//Set keys and flags to default
			currKey_=-1;
			remKey_=-1;
			loopFlag_=false;
			//Return True - Insertion of record completed
			return true;
		}
		//If no removed record has been traversed over, the insertion shall
		//continue into the current record.
		else{
			//Add record with key and value
			records_[currKey_]=new Record(key,value);
			//Increment the amount of records within the table
			size_++;
			//Set keys and flags to default
			currKey_=-1;
			remKey_=-1;
			loopFlag_=false;
			//Return True - Insertion of record completed
			return true;
		}
	}
	else{
		//Set keys and flags to default
		remKey_=-1;
		currKey_=-1;
		loopFlag_=false;
		//Return False - update and insertion have failed
		return false;
	}
}
//This function takes in one argument: a string called key.
//The key is used to determine the collision key as well as
//confirm if the key already exists within the table of records
//and is ready for removal. If this record is being removed,
//the size counter would decrement and the remFlag_ within the
//record would be set to true to indiciate that this record has
//been marked as deleted.
//This function will return a boolean:
//True - if the key has been found within the table of records
//False - if the key has not been found within the table of records
template <class TYPE>
bool LPTable<TYPE>::remove(const string& key){
	if(find(key,foundValue_)){
		//Set the remFlag_ to true to indiciate as removed record
		records_[currKey_]->remFlag_=true;
		//Decrement the amount of records within the table
		size_--;
		//Set keys and flags to default
		remKey_=-1;
		currKey_=-1;
		loopFlag_=false;
		//Return true - removal completed
		return true;
	}
	else{
		//Set keys and flags to default
		remKey_=-1;
		currKey_=-1;
		loopFlag_=false;
		//Return false - removal failed
		return false;
	}
}

//This function takes in two arguments: a string called key
//and a templated data type called value. The key is used
//to determine the collision key as well as confirm if it
//already exists within the table of records. The value will
//be changed and sent back if the key has been found within
//the records.
//This function will return a booleab:
//True - if an acitve record with a matching key has been found.
//False - if the initialization of the colKey_ has failed
//				or if a null records has been found instead of the key.
template <class TYPE>
bool LPTable<TYPE>::find(const string& key, TYPE& value){
	//Check to see if the colKey_ is set and if the hash
	//of the key is outside the range of the table.
	//Returns false if the hash has not been accepted for
	//initializtion. Otherwise currKey_ is set to colKey_.
	if(colKey_==-1){
		colKey_=getHashedId(key);
		if(colKey_>=maxOpen_){
			colKey_=-1;
			return false;
		}
		currKey_=colKey_;
	}
	//Checks to see if the records at the currKey_ is equal to nullptr
	if(records_[currKey_]==nullptr){
		colKey_=-1;
		return false;
	}
	//Checks to see if the remFlag_ has been set to true within the
	//current record and that this specific else if statement
	//has only been executed once by checking to see if the loopFlag_
	//is set to false. If they are true and false respectively,
	//then the remKey_ will be set to the current record, the loopFlag_
	//will be set to true so this statement will fail for the remainder
	//of the recursion. This is done to ensure that the first dead record
	//is saved for the use of the update function.
	else if(records_[currKey_]->remFlag_ && !loopFlag_){
		remKey_=currKey_;
		currKey_++;
		loopFlag_=true;
		return find(key,value);
	}
	//If the current record's key is the same as the key from this function's
	//arguments, and the remFlag_ within the current flag is set to false,
	//the inputted value argument shall be changed to the data of the current
	//record and true is returned, which indicates that find has found the
	//key.
	else if(records_[currKey_]->key_==key && !records_[currKey_]->remFlag_){
		value=records_[currKey_]->data_;
		colKey_=-1;
		return true;
	}
	//If the currKey_ has reached the end of the array of size maxOpen_ and
	//requires to continue traversing the hash table, then the currKey_ will
	//be set to the beginning of the array of records and continue to traverse
	//the array of records by recursively recalling itself, find().
	else if(currKey_==(maxOpen_-1)){
		currKey_=0;
		return find(key,value);
	}
	//If all other statements have failed, the traversal of the hash table
	//will continue by means of iterating the currKey_ forward and recursively
	//calling itself, find().
	else{
		currKey_++;
		return find(key,value);
	}

}

//This is the Copy Operator which returns the the copied table
template <class TYPE>
const LPTable<TYPE>& LPTable<TYPE>::operator=(const LPTable<TYPE>& other){
	//Continue if this table does not already equal to the other table
	if(this!=&other){
		//If this table has an active array of records, they shall be dynamically deleted
		if(records_ || maxOpen_!=other.maxOpen_){
			//Setting currKey_ for while loop
			currKey_=0;
			while (currKey_<maxOpen_) {
				delete records_[currKey_++];
			}
			delete [] records_;
		}
		//Ensure records is nulled
		records_=nullptr;
		///Sets the maximum size of the array
		maxOpen_=other.maxOpen_;
		//Initializes the array of records to maxOpen_
		records_=new Record*[maxOpen_];
		//Sets the max amount of allowed records
		max_=other.max_;
		//Sets the amount of active records that are being passed over from the other table
		size_=other.size_;
		//Setting currKey_ for while loop
		currKey_=0;
		//Dynamically adds the other table's records to this table
		while (currKey_<maxOpen_) {
			records_[currKey_]=other.records_[currKey_];
			currKey_++;
		}
		//Set keys and flags to default
		colKey_=-1;
		loopFlag_=false;
		remKey_=-1;
		currKey_=-1;
	}
	//Returning the newly copyied table
	return *this;
}
//This is the Move Operator which returns the moved table
template <class TYPE>
const LPTable<TYPE>& LPTable<TYPE>::operator=(LPTable<TYPE>&& other){
	//Continue if this table does not already equal to the other table
	if(this!=&other){
		//This will perform a quick swap of information between table
		swap(records_,other.records_);
		swap(max_,other.max_);
		swap(size_,other.size_);
		swap(maxOpen_,other.maxOpen_);
		//Set keys and flags to default
		remKey_=-1;
		loopFlag_=false;
		colKey_=-1;
		currKey_=-1;
	}
	//Returned the newly moved table
	return std::move(*this);
}
//This is the Destructor
template <class TYPE>
LPTable<TYPE>::~LPTable(){
	//If the array of records is active, dynamically de-allocate all resources
	//and then delete array of records.
	if(records_){
		//Setting currKey_ for while loop
		currKey_=0;
		while(currKey_<maxOpen_){
			records_[currKey_++]=nullptr;
		}
	}
	//Delete records
	delete records_;
}
