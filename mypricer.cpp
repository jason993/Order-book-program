// This is an open question which can be found at: 
// http://rgmadvisors.com/problems/orderbook/

#include <iostream>
#include <string>
#include <float.h>
#include <assert.h>
#include <iomanip>
#include <map>
#include <memory>
#include <cmath>
#include <unordered_map>
#include <sstream>
#include <tuple>

using namespace std;

class Order{
	public:
	int timestamp;
	char order_type;
	string order_id;
	char side;
	double price;
	int size;
	
	Order() {}
	Order(const Order &om) : timestamp (om.timestamp),
				order_type (om.order_type),
				order_id (om.order_id),
				side (om.side),
				price(om.price),
				size (om.size) {}

};



typedef tuple<double, int, char> PriceSizeSide;
typedef map<double, int> Book_t;
typedef unordered_map<string, PriceSizeSide> Search_t;



istream &operator >> (istream &is, Order &order){
	is >> order.timestamp;
	is >> order.order_type;
//	order.order_type = toupper(order.order_type);
	is >> order.order_id;


	if(order.order_type == 'A')
	{
		is >> order.side;
		is >> order.price;
		if(order.price < 0.0)
		{
			cerr << "order price should be positive number" <<endl;
		}

	}
	else
	{

		order.side = ' ';
		order.price = 0.0;
	}

	is >> order.size;
	if(order.size <= 0)
	{
		cerr << "order size shouble be positve number" <<endl;
	}
	return is;
}

inline bool isEqual(const double &lhs, const double &rhs)
{
	return (abs(lhs - rhs) < 1e-5);
}


inline double expense(Book_t &askBook, const int &targetSize, double &askLocator)
{
	    int currSize = targetSize;
		double price = 0;
        Book_t::iterator it = askBook.begin();

		while(currSize > 0 && it != askBook.end())
		{
			int size = min(currSize, it->second);
			price += size * (it->first);
			currSize -= size;
			++it;
		}
		
		askLocator = (--it)->first;
    	return price;

}

inline double income(Book_t &bidBook, const int &targetSize, double &bidLocator)
{
		
	 	int currSize = targetSize;
		double price = 0;
        Book_t::reverse_iterator it = bidBook.rbegin();

		while(currSize > 0 && it != bidBook.rend())
		{
			int size = min(currSize, it->second);
			price += size * (it->first);
			currSize -= size;
			++it;
		}
		bidLocator = (--it)->first;
		return price;
			
}



void Addorder (Search_t &search, Book_t &askBook, Book_t &bidBook,  Order &order, int &askBookSize, int &bidBookSize, int &targetSize, 
		                double &askLocator, double &bidLocator, double &askTotal, double &bidTotal)
{
	if(order.side == 'B')
	{
		bidBook[order.price] += order.size;
		search[order.order_id] = PriceSizeSide(order.price, order.size, order.side);
		bidBookSize += order.size;
		
 		if( bidBookSize < targetSize)    //not enough size to calculate the income
		{	
			return;
		}	
		if(isEqual(bidLocator, 0.0)  || (bidLocator > 0.0 && !(order.price < bidLocator)))
		{

			double newIncome = income(bidBook, targetSize, bidLocator);

			if (newIncome != bidTotal)
			{
				cout << order.timestamp << " S " << newIncome <<endl;
				bidTotal = newIncome;
			}	
		}
	}
    else
	{
		askBook[order.price] += order.size;
		search[order.order_id] = PriceSizeSide(order.price, order.size, order.side);
		askBookSize += order.size;
		
		if(askBookSize < targetSize)  //not enough size to calculate the expense
			return;

        if(isEqual(askLocator, 0.0) || (askLocator > 0.0 && !(askLocator < order.price)))
		{
			double newExpense = expense(askBook, targetSize, askLocator);

			if (newExpense != askTotal)
			{
				cout << order.timestamp << " B " << newExpense <<endl;
				askTotal = newExpense;
			}	
		}	
	}
} 
    


void Reduceorder(Search_t &search, Book_t &askBook, Book_t &bidBook,  Order &order, int &askBookSize, int &bidBookSize, int &targetSize, 
		                double &askLocator, double &bidLocator, double &askTotal, double &bidTotal, PriceSizeSide &reduceOrderInfo)
{
	int reduceSize = min(get<1>(reduceOrderInfo), order.size);

   	if (get<2>(reduceOrderInfo) == 'B')
	{
		get<1>(reduceOrderInfo) -= reduceSize;
		bidBookSize  -= reduceSize;
		bidBook[get<0>(reduceOrderInfo)] -= reduceSize; 
		
		if (bidBook[get<0>(reduceOrderInfo)] == 0)
        	bidBook.erase(get<0>(reduceOrderInfo));

		if(bidBookSize < targetSize && bidTotal != .0)  // not enough size after reducing the order
		{
			cout << order.timestamp << " S NA" << endl;
			bidTotal = 0.0;	bidLocator = 0.0;
			
		}
		else if(bidBookSize >= targetSize && !(get<0>(reduceOrderInfo) < bidLocator))
		{
			double newIncome = income(bidBook, targetSize, bidLocator);

			if (newIncome != bidTotal)
			{
				cout << order.timestamp << " S " << newIncome <<endl;
				bidTotal = newIncome;
			}

		}		
	}	
	else
    {
		get<1>(reduceOrderInfo) -= reduceSize;
		askBookSize  -= reduceSize;
		askBook[get<0>(reduceOrderInfo)] -= reduceSize; 
		
		if (askBook[get<0>(reduceOrderInfo)] == 0)
        	askBook.erase(get<0>(reduceOrderInfo));

		if(askBookSize < targetSize && askTotal != .0) // not enough size after reducing the order
		{
			cout << order.timestamp << " B NA" << endl;
			askTotal = 0.0; askLocator = 0.0;
		}
		else if( askBookSize >= targetSize && !(get<0>(reduceOrderInfo) > askLocator))
		{
			double newExpense = expense(askBook, targetSize, askLocator);

			if (newExpense != askTotal)
			{
				cout << order.timestamp << " B " << newExpense <<endl;
				askTotal = newExpense;
			}
		}
	}	

	if(get<1>(reduceOrderInfo) == 0)
		search.erase(order.order_id);
    
}


int main (int argc, char **argv)
{
	if(argc != 2)
	{
		cout << "usage: "<< argv[0] << "<targetSize>" <<endl;
	    return -1;
	}
	cout << setiosflags(ios::fixed) << setprecision(2);

	stringstream ss (argv[1]);
	int targetSize; 
	ss >> targetSize;

	Book_t bidBook, askBook;
	Search_t search;

	double bidTotal = 0.0;
	double askTotal = 0.0;

    //To locator the lateset range of the price 
	double bidLocator = 0.0;
	double askLocator = 0.0; 

	int bidBookSize = 0;
	int askBookSize = 0;

	Order order;
	while(cin >> order)
	{
		if(order.order_type == 'A')
		{
			 Addorder(search, askBook, bidBook, order, askBookSize, bidBookSize, targetSize, 
		               askLocator, bidLocator, askTotal, bidTotal);
			
		}
		else
		{
		//	assert(order.order_type == 'R');
            PriceSizeSide reduceOrderInfo = search[order.order_id];

      		Reduceorder(search, askBook, bidBook, order, askBookSize, bidBookSize, targetSize, 
		                askLocator, bidLocator, askTotal, bidTotal, reduceOrderInfo);
     
		}
	}
  
	return 0;
}




