/* -*- c++ -*-
 * $Id: interlock.cc 13705 2020-07-20 21:46:53Z greg $
 *
 * Call-chain/interlock finder.
 *
 * Copyright the Real-Time and Distributed Systems Group,
 * Department of Systems and Computer Engineering,
 * Carleton University, Ottawa, Ontario, Canada. K1S 5B6
 *
 * April 2003
 *
 * ------------------------------------------------------------------------
 */

#include <algorithm>
#include <cmath>
#include "lqns.h"
#include <algorithm>
#include "interlock.h"
#include "task.h"
#include "entry.h"
#include "option.h"
#include "model.h"

/* -------------------------------------------------------------------- */
/* Funky Formatting functions for inline with <<.			*/
/* -------------------------------------------------------------------- */

class StringNManip {
public:
    StringNManip( ostream& (*ff)(ostream&, const std::string&, const unsigned ), const std::string& s, const unsigned n ) : f(ff), _s(s), _n(n) {}
private:
    ostream& (*f)( ostream&, const std::string&, const unsigned );
    const std::string& _s;
    const unsigned int _n;

    friend ostream& operator<<(ostream & os, const StringNManip& m ) { return m.f(os,m._s,m._n); }
};

StringNManip trunc( const std::string&, const unsigned );

/************************************************************************/
/*                     Throughput Interlock Model.                      */
/************************************************************************/

/*
 * Generate the interlock table.  It is impossible to interlock on
 * infinite servers since all calls (by definition) go to unique
 * instances.
 */

Interlock::Interlock( const Entity * aServer ) 
    : commonEntries(),
      allSourceTasks(),
      ph2SourceTasks(),
      myServer(aServer), 
      sources(0)
{
    initialize();
}



Interlock::~Interlock()
{
    commonEntries.clear();
    allSourceTasks.clear();
    ph2SourceTasks.clear();
}


void
Interlock::initialize()
{
    if ( !myServer->isInfinite() ) {
	findInterlock();
	pruneInterlock();
	findSources();
    }
}


/*
 * This function searches for interlock paths among all the callers
 * to `aTask'.
 */

void
Interlock::findInterlock()
{
    if ( Options::Debug::interlock() ) {
	cout << "Interlock for server: " << myServer->name() << endl;
    }

    /* Locate all callers to myServer */

    std::set<Task *> myClients;
    myServer->clients( myClients );

    for ( std::set<Task *>::const_iterator clientA = myClients.begin(); clientA != myClients.end(); ++clientA ) {
	if ( !(*clientA)->isUsed() ) continue;		/* Ignore this task - not used. */

	for ( std::set<Task *>::const_iterator clientC = myClients.begin(); clientC != myClients.end(); ++clientC ) {
	    if ( (*clientA) == (*clientC) || !(*clientC)->isUsed() ) continue;

	    for ( std::vector<Entry *>::const_iterator entryA = (*clientA)->entries().begin(); entryA != (*clientA)->entries().end(); ++entryA ) {
		for ( std::vector<Entry *>::const_iterator entryC = (*clientC)->entries().begin(); entryC != (*clientC)->entries().end(); ++entryC ) {
		    bool foundAB = false;
		    bool foundCD = false;

		    /* Check that both entries call me. */

		    for ( std::vector<Entry *>::const_iterator entry = myServer->entries().begin(); entry != myServer->entries().end(); ++entry ) {
			if ( (*entryA)->isInterlocked( (*entry) ) ) foundAB = true;
			if ( (*entryC)->isInterlocked( (*entry) ) ) foundCD = true;
		    }
		    if ( foundAB && foundCD ) {
			findParentEntries( *(*entryA), *(*entryC) );
		    }
		}
	    }
	}
    }
}




/*
 * Locate all common parent entries to A and C.
 */

void
Interlock::findParentEntries( const Entry& srcA, const Entry& srcC )
{
    const Entry& entryA = srcA;
    const Entry& entryC = srcC;
    const unsigned a = entryA.entryId();
    const unsigned c = entryC.entryId();

    if ( Options::Debug::interlock() ) {
	cout << "  Common parents for entries " << srcA.name() << " and " << srcC.name() << ": ";
    }

    /* Figure 6 in interlock paper. */

    for ( std::set<Task *>::const_iterator aTask = Model::__task.begin(); aTask != Model::__task.end(); ++aTask ) {

	/* x calls a, a calls b; y calls c, c calls d */

	for ( std::vector<Entry *>::const_iterator srcX = (*aTask)->entries().begin(); srcX != (*aTask)->entries().end(); ++srcX ) {
	    for ( std::vector<Entry *>::const_iterator srcY = (*aTask)->entries().begin(); srcY != (*aTask)->entries().end(); ++srcY ) {
		if ( (*srcX)->_interlock[a].all > 0.0 && (*srcY)->_interlock[c].all > 0.0 ) {

		    /* Prune here (branch point?) */

		    if ( Options::Debug::interlock() ) {
			cout << (*srcX)->name();
		    }

		    if ( isBranchPoint( *(*srcX), entryA, *(*srcY), entryC ) ) {
			commonEntries.insert(*srcX);
			if ( Options::Debug::interlock() ) {
			    cout << "* ";
			}
		    } else if ( Options::Debug::interlock() ) {
			cout << " ";
		    }
		}
	    }
	}
    }

    if ( Options::Debug::interlock() ) {
	cout << endl;
    }
}



/*
 * Given a set of entries that are branch points, find interlocked flow.
 * Interlock probability is affected by the number of threads of viaTask.
 * This method is called each time we generate an MVA model.
 */

double
Interlock::interlockedFlow( const Task& viaTask ) const
{
    if ( sources == 0 ) return 0.0;

    /* Find all flow from the common parent list to viaTask. */

    double sum = 0.0;
    for ( std::set<const Entry *>::const_iterator srcE = commonEntries.begin(); srcE != commonEntries.end(); ++srcE ) {
	for ( std::vector<Entry *>::const_iterator dstA = viaTask.entries().begin(); dstA != viaTask.entries().end(); ++dstA ) {
	    const Entity * srcTask = (*srcE)->owner();
	    const unsigned a = (*dstA)->entryId();

	    if ( (*srcE)->_interlock[a].all > 0.0 && (*srcE)->maxPhase() == 1 && allSourceTasks.find( srcTask ) != allSourceTasks.end() ) {
		sum += (*srcE)->throughput() * (*srcE)->_interlock[a].all;
	    } else if ((*srcE)->_interlock[a].ph1 > 0.0 && (*srcE)->maxPhase() > 1 && allSourceTasks.find( srcTask )  != allSourceTasks.end() ){
		sum += (*srcE)->throughput() * (*srcE)->_interlock[a].ph1;	
	    }
		
	    const double ph2 = (*srcE)->_interlock[a].all - (*srcE)->_interlock[a].ph1;
	    if ( ph2 > 0.0 && ph2SourceTasks.find( srcTask ) != ph2SourceTasks.end() ) {
		sum += (*srcE)->throughput() * ph2;
	    }
	    if ( flags.trace_interlock ) {
		cout << "  Interlock E=" << (*srcE)->name() << " A=" << (*dstA)->name() 
		     << " Throughput=" << (*srcE)->throughput() 
		     << ", interlock[" << a << "]={" << (*srcE)->_interlock[a].all << "," << ph2
		     << "}, sum=" << sum << endl;
	    }
	}
    }

    if ( flags.trace_interlock ) {
	cout << "Interlock sum=" << sum << ", viaTask: " << viaTask.throughput() 
	     << ", threads=" << viaTask.concurrentThreads()  << ", sources=" << sources << endl;
    }
    return min( sum, viaTask.throughput() ) / (viaTask.throughput() * viaTask.concurrentThreads() * sources );
}



/*
 * Go through the interlock list and remove entries from parents 
 * See prune above.
 */

void
Interlock::pruneInterlock()
{
    /* For all tasks in common entry... subtract off their common entries */

    std::set<const Entry *> prune;
    for ( std::set<const Entry *>::const_iterator i = commonEntries.begin(); i != commonEntries.end(); ++i ) {
	const Entity * dst = (*i)->owner();
	if ( !dst->interlock ) continue;

	const std::set<const Entry *>& dst_entries = dst->interlock->commonEntries;
	for ( std::set<const Entry *>::const_iterator entry = dst_entries.begin(); entry != dst_entries.end(); ++entry ) {
	    if ( commonEntries.find( *entry ) != commonEntries.end() ) {
		prune.insert( *entry );
	    }
	}
    }
    commonEntries = difference<const Entry *>( commonEntries, prune );
}



/*
 * Find all sources for flow along interlock paths by descending down the
 * call paths to ``myServer'' or some other terminus.  Only count those
 * paths which actually go to ``myServer''.
 */

void
Interlock::findSources()
{
    std::set<const Entity *> interlockedTasks;

    /* Look for all parent tasks */

    std::deque<const Entry *> entryStack;
    for ( std::set<const Entry *>::const_iterator entry = commonEntries.begin(); entry != commonEntries.end(); ++entry ) {
	const Entity * aTask = (*entry)->owner();

	/* Add tasks corresponding to branch point entries */

	allSourceTasks.insert( aTask );

	if ( (*entry)->maxPhase() > 1 ) {
	    ph2SourceTasks.insert( aTask );
	}

	/* Locate all tasks on interlocked paths. */

	(*entry)->getInterlockedTasks( entryStack, myServer, interlockedTasks );
    }

    /*
     * Prune out tasks in sourceTasks that are also in
     * interlockedTasks for allSrcTasks.  Ph2Tasks is the opposite
     * of what was prunded out as phase 2 sources are independent.
     */

#ifdef	DEBUG_INTERLOCK
    if ( Options::Debug::interlock() ) {
	cout <<         "    All Sourcing Tasks: ";
	for ( std::set<const Entity *>::const_iterator task = allSourceTasks.begin(); task != allSourceTasks.end(); ++task ) {
	    cout << (*task)->name() << " ";
	}
	cout << endl << "    Interlocked Tasks:  ";
	for ( std::set<const Entity *>::const_iterator task = interlockedTasks.begin(); task != interlockedTasks.end(); ++task ) {
	    cout << (*task)->name() << " ";
	}
	cout << endl;
    }
#endif

    allSourceTasks = difference<const Entity *>( allSourceTasks, interlockedTasks );
    ph2SourceTasks = intersection<const Entity *>( ph2SourceTasks, interlockedTasks );

#ifdef	DEBUG_INTERLOCK
    if ( Options::Debug::interlock() ) {
	cout <<         "    Common Parent Tasks (all): ";
	for ( std::set<const Entity *>::const_iterator task = allSourceTasks.begin(); task != allSourceTasks.end(); ++task ) {
	    cout << (*task)->name() << " ";
	}
	cout << endl << "    Common Parent Tasks (Ph2): ";
	for ( std::set<const Entity *>::const_iterator task = ph2SourceTasks.begin(); task != ph2SourceTasks.end(); ++task ) {
	    cout << (*task)->name() << " ";
	}
	cout << endl;
    }
#endif

    /* Now count up the instances on each task in sourceTasks */
    /* And add on all other sources */

    sources = countSources( interlockedTasks );

    /* Useful trivia. */

    if ( Options::Debug::interlock() ) {
	for ( std::set<const Entity *>::const_iterator task = interlockedTasks.begin(); task != interlockedTasks.end(); ++task ) {
	    cout << (*task)->name() << " ";
	}
	cout << endl << "    Sourcing Tasks:    ";
	for ( std::set<const Entity *>::const_iterator task = allSourceTasks.begin(); task != allSourceTasks.end(); ++task ) {
	    cout << (*task)->name() << " ";
	}
	cout << endl << endl;
    }
}



/*
 * Return 1 if X and Y follow paths to different tasks on the
 * correct call path.
 */

bool
Interlock::isBranchPoint( const Entry& srcX, const Entry& entryA, const Entry& srcY, const Entry& entryB ) const
{
    /*
     * I have to ensure that a call to myself (which is not feasible...)
     */

    if ( srcX.owner() == entryA.owner()
	 && srcY.owner() == entryB.owner()
	 && entryA.owner() == entryB.owner() ) return false;	// Multiserver client

    /*
     * Quick check -- send interlock?
     */

    if ( srcX == entryA || srcY == entryB ) return true;

#ifdef	NOTDEF
    if ( Options::Debug::interlock() ) {
	cout << "  isBranchPoint: " << srcX.name() << (char *)(srcX.isProcessorEntry() ? "*, " : ", ")
	     << entryA.name() << (char *)(entryA.isProcessorEntry() ? "*, " : ", ")
	     << srcY.name() << (char *)(srcY.isProcessorEntry() ? "*, " : ", ")
	     << entryB.name() << (char *)(entryB.isProcessorEntry() ? "*, " : ", ")
	     << endl;
    }
#endif

    /*
     * Sequence over all calls from X and Y and see if they go to
     * different tasks.  Only consider those entries which ultimately
     * go to a and b respectively
     */

    CallInfo nextX( srcX, Call::RENDEZVOUS_CALL );
    CallInfo nextY( srcY, Call::RENDEZVOUS_CALL );

    const unsigned a = entryA.entryId();
    const unsigned b = entryB.entryId();

    for ( std::vector<CallInfoItem>::const_iterator yxe = nextX.begin(); yxe != nextX.end(); ++yxe ) {
	const Entry * dstE = yxe->dstEntry();

	if ( dstE->_interlock[a].all == 0.0 ) continue;

	for ( std::vector<CallInfoItem>::const_iterator yyf = nextY.begin(); yyf != nextY.end(); ++yyf ) {
	    const Entry * dstF = yyf->dstEntry();

	    if ( dstF->_interlock[b].all > 0.0 && dstE->owner() != dstF->owner() ) return 1;
	}
    }

    return false;
}



/*
 * This procedure is used to locate all of the other sources that are along the
 * call paths.  Tasks found cannot be in sources or in paths.
 */

unsigned
Interlock::countSources( const std::set<const Entity *>& interlockedTasks )
{
    /*
     * Sequence over arriving arcs and add task to sources
     * provided that it is not in paths.
     */

    for ( std::set<const Entity *>::const_iterator task = interlockedTasks.begin(); task != interlockedTasks.end(); ++task ) {
	const std::vector<Entry *>& entries = (*task)->entries();
	for ( std::vector<Entry *>::const_iterator entry = entries.begin(); entry != entries.end(); ++entry ) {
	    const std::set<Call *>& callerList = (*entry)->callerList();
	    for ( std::set<Call *>::const_iterator call = callerList.begin(); call != callerList.end(); ++call ) {
		if ( !(*call)->hasRendezvous() ) continue;

		const Task * parentTask = (*call)->srcTask();
		if ( interlockedTasks.find( parentTask ) == interlockedTasks.end() ) {
		    allSourceTasks.insert( parentTask );
		}
	    }
	}
    }

    /*
     * Now count up all the copies of sourcing tasks.  Note that we
     * have to use the special population() call to account
     * properly for infinite servers.
     */

    unsigned n = for_each( allSourceTasks.begin(), allSourceTasks.end(), Sum<const Entity,double>( &Entity::population ) ).sum();

    /*
     * Add in phase 2 sources of tasks that are on the interlock
     * path.  These tasks count as additional sources because phase
     * 2+ flow is (quasi) independent of phase 1 flow.  Ignore all
     * tasks that are immediate parents of the interlockee (mva
     * interlock attends to this case).
     */

    const std::vector<Entry *>& dst_entries = myServer->entries();
    for ( std::set<const Entity *>::const_iterator task = interlockedTasks.begin(); task != interlockedTasks.end(); ++task ) {
	const std::vector<Entry *>& src_entries = (*task)->entries();
	for ( std::vector<Entry *>::const_iterator srcEntry = src_entries.begin(); srcEntry != src_entries.end(); ++srcEntry ) {
	    for ( std::vector<Entry *>::const_iterator dstEntry = dst_entries.begin(); dstEntry != dst_entries.end(); ++dstEntry ) {
		const unsigned e = (*dstEntry)->entryId();
		const double ph2 = (*srcEntry)->_interlock[e].all - (*srcEntry)->_interlock[e].ph1;
		if ( ph2 > 0.0 ) {
		    n += 1;
		    goto nextTask;	/* Any hit counts... */
		}
	    }
	}
    nextTask: ;
    }

    /* All done! */

    return n;
}



/*
 * Print path information.
 */

ostream&
Interlock::print( ostream& output ) const
{
    output << myServer->name() << ": Sources=" << sources << ", entries: " ;

    for ( std::set<const Entry *>::const_iterator srcE = commonEntries.begin(); srcE != commonEntries.end(); ++srcE ) {
	output << (*srcE)->name() << " ";
    }

    return output;
}

/*
 * Print out path table.
 */

ostream&
Interlock::printPathTable( ostream& output )
{
    std::set<Entry *>::const_iterator srcEntry;
    std::set<Entry *>::const_iterator dstEntry;
    static const unsigned int columns = 5;

    ios_base::fmtflags oldFlags = output.setf( ios::left, ios::adjustfield );
    output << "src\\dst   ";
    unsigned i;
    unsigned j;
    for ( i = 1, srcEntry = Model::__entry.begin(); srcEntry != Model::__entry.end(); ++srcEntry, ++i ) {
	const Entry * anEntry = *srcEntry;
	output << trunc( anEntry->name(), 10 );
	if ( i % columns == 0 ) {
	    output << ' ';
	}
    }
    output << endl;

    for ( i = 0, srcEntry = Model::__entry.begin(); srcEntry != Model::__entry.end(); ++srcEntry, ++i ) {
	const Entry * src = *srcEntry;

	if ( i % columns == 0 ) {
	    output << "----------";
	    for ( j = 1; j <= Model::__entry.size(); ++j ) {
		output << "----------";
		if ( j % columns == 0 ) {
		    output << '+';
		}
	    }
	    output << endl;
	}

	output << trunc( src->name(), 10 );
	for ( j = 1, dstEntry = Model::__entry.begin(); dstEntry != Model::__entry.end(); ++dstEntry, ++j ) {
	    output.setf( ios::right, ios::adjustfield );
	    output << setw(4) << src->_interlock[(*dstEntry)->entryId()].all << ",";
	    output.setf( ios::left, ios::adjustfield );
	    output << setw(4) << src->_interlock[(*dstEntry)->entryId()].ph1 << " ";
	    if ( j % columns == 0 ) {
		output << '|';
	    }
	}
	output << endl;
    }
    output.flags(oldFlags);
    output << endl;

    return output;
}

bool 
InterlockInfo::operator==( const InterlockInfo& arg ) const
{
    return all == arg.all && ph1 == arg.ph1;
}

ostream&
operator<<( ostream& output, const InterlockInfo& self )
{
    output << self.all << self.ph1 << endl;
    return output;
}


InterlockInfo&
InterlockInfo::operator=( const InterlockInfo& anEntry )
{
    if ( this == &anEntry ) return *this;

    ph1 = anEntry.ph1;
    all = anEntry.all;

    return *this;
}



InterlockInfo&
operator+=( InterlockInfo& arg1, const InterlockInfo& arg2 )
{
    arg1.ph1 += arg2.ph1;
    arg1.all += arg2.all;

    return arg1;
}



InterlockInfo&
operator-=( InterlockInfo& arg1, const InterlockInfo& arg2 )
{
    arg1.ph1 -= arg2.ph1;
    arg1.all -= arg2.all;

    return arg1;
}


InterlockInfo
operator*( const InterlockInfo& multiplicand, double multiplier )
{
    InterlockInfo product;

    product.all = multiplicand.all * multiplier;
    product.ph1 = multiplicand.ph1 * multiplier;

    return product;
}

bool 
operator>( const InterlockInfo& lhs, double rhs )
{
    return ((lhs.all>rhs) || (lhs.ph1>rhs));
}

static ostream& trunc_str( ostream& output, const std::string& s, const unsigned n ) 
{
    if ( s.size() > n ) {
	output.write( s.c_str(), n );
    } else {
	ios_base::fmtflags oldFlags = output.setf( ios::left, ios::adjustfield );
	output << setw(n) << setfill( ' ' ) << s;
	output.flags(oldFlags);
    }
    return output;
}

StringNManip trunc( const std::string& s, const unsigned n ) { return StringNManip( trunc_str, s, n ); }
