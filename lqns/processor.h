/* -*- c++ -*-
 * $HeadURL: http://rads-svn.sce.carleton.ca:8080/svn/lqn/trunk-V5/lqns/processor.h $
 *
 * Processors.
 *
 * Copyright the Real-Time and Distributed Systems Group,
 * Department of Systems and Computer Engineering,
 * Carleton University, Ottawa, Ontario, Canada. K1S 5B6
 *
 * November, 1994
 * May, 2009
 *
 * ------------------------------------------------------------------------
 * $Id: processor.h 13676 2020-07-10 15:46:20Z greg $
 * ------------------------------------------------------------------------
 */

#if	!defined(PROCESSOR_H)
#define PROCESSOR_H

#include "dim.h"
#include <lqio/dom_processor.h>
#include <set>
#include <cstring>
#include <string>
#include "entity.h"

class Task;
class Processor;
class Server;
class Group;

class Processor : public Entity {

    class SRVNManip {
    public:
	SRVNManip( ostream& (*ff)(ostream&, const Processor & ), const Processor & theProcessor ) : f(ff), aProcessor(theProcessor) {}
    private:
	ostream& (*f)( ostream&, const Processor& );
	const Processor & aProcessor;

	friend ostream& operator<<(ostream & os, const SRVNManip& m ) { return m.f(os,m.aProcessor); }
    };

public:
    static void create( const std::pair<std::string,LQIO::DOM::Processor*>& );

protected:
    Processor( LQIO::DOM::Processor* );

public:
    virtual ~Processor();

    /* Initialization */

    virtual bool check() const;
    virtual Processor& configure( const unsigned );
    virtual Processor& initPopulation();

    /* Instance Variable access */

    virtual LQIO::DOM::Processor * getDOM() const { return dynamic_cast<LQIO::DOM::Processor *>(Entity::getDOM()); }
    virtual double rate() const;
    virtual unsigned int fanOut( const Entity * ) const;
    virtual unsigned int fanIn( const Task * ) const;

    /* Queries */

    virtual bool isProcessor() const { return true; }
    bool isInteresting() const;
    virtual unsigned nClients() const { return _tasks.size(); }
    virtual bool hasVariance() const;
    bool hasPriorities() const;
    virtual unsigned validScheduling() const;

    /* Model Building. */

    Processor& addGroup( Group * aGroup ) { _groups.insert( aGroup ); return *this; }
    const std::set<Group *>& groups() const { return _groups; }

    Server * makeServer( const unsigned nChains );

    /* DOM insertion of results */

    virtual const Processor& insertDOMResults() const;
    virtual ostream& print( ostream& ) const;

public:
    static Processor * find( const std::string& name );

private:
    SRVNManip print_processor_type() const { return SRVNManip( output_processor_type, *this ); }
    static ostream& output_processor_type( ostream& output, const Processor& aProcessor );
    
public:
    static bool __prune;

private:
    std::set<Task *> _tasks;			/* List of processor's tasks	*/
    std::set<Group *> _groups;			/* List of processor's Group	*/
};


/*
 * Special processor class which is used to implement delays in the
 * model such as phase think times.  There is no DOM attached to it,
 * so we have to fake out some of the get methods.
 */

class DelayServer : public Processor
{
public:
    DelayServer() : Processor( 0 ) {}		/* No Dom */
    virtual bool check() const { return true; }

    virtual double rate() const { return 1.0; }
    virtual const std::string& name() const { static const std::string s="DELAY"; return s; }
    virtual scheduling_type scheduling() const { return SCHEDULE_DELAY; }
    virtual unsigned copies() const { return 1; }
    virtual unsigned replicas() const { return 1; }
    virtual bool isInfinite() const { return true; }

    virtual const DelayServer& insertDOMResults() const { return *this; }	/* NOP */
};
#endif
