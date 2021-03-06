/* -*- c++ -*-
 * $HeadURL: http://rads-svn.sce.carleton.ca:8080/svn/lqn/trunk-V5/lqns/entity.h $
 *
 * Pure virtual class for tasks and processors.
 *
 * Copyright the Real-Time and Distributed Systems Group,
 * Department of Systems and Computer Engineering,
 * Carleton University, Ottawa, Ontario, Canada. K1S 5B6
 *
 * November, 1994
 *
 * $Id: entity.h 13741 2020-08-06 04:19:44Z greg $
 *
 * ------------------------------------------------------------------------
 */

#if	!defined(ENTITY_H)
#define ENTITY_H

#include "dim.h"
#include <vector>
#include <lqio/input.h>
#include <lqio/dom_processor.h>
#include "vector.h"
#include "prob.h"
#include "call.h"

class Entity;
class Entry;
class Processor;
class Format;
class Interlock;
class Task;
class Model;
class Server;
class Submodel;

typedef Vector<unsigned> ChainVector;

ostream& operator<<( ostream&, const Entity& );
int operator==( const Entity&, const Entity& );

#define TYPE_STR_BUFSIZ	20

/* ----------------------- Abstract Superclass ------------------------ */

class Entity {
    friend class Generate;

public:
    /*
     * Compare two entities by their submodel. 
     */

    struct LT
    {
	bool operator()(const Entity * e1, const Entity * e2) const {
#if 1
	    return (e1->submodel() < e2->submodel()) 
		|| (e1->getDOM() && (!e2->getDOM() || e1->getDOM()->getSequenceNumber() < e2->getDOM()->getSequenceNumber() ));
#else
	    return (e1->submodel() < e2->submodel())
		|| e1->isTask() && e2->isProcessor()
		|| (e1->getDOM() && (!e2->getDOM() || e1->getDOM()->getName() < e2->getDOM()->getName() ));
#endif
	}
    };


private:
    class SRVNManip {
    public:
	SRVNManip( ostream& (*f)(ostream&, const Entity & ), const Entity & entity ) : _f(f), _entity(entity) {}
    private:
	ostream& (*_f)( ostream&, const Entity& );
	const Entity & _entity;

	friend ostream& operator<<(ostream & os, const SRVNManip& m ) { return m._f(os,m._entity); }
    };

public:
    Entity( LQIO::DOM::Entity* domVersion, const std::vector<Entry *>& entries );
    virtual ~Entity();

private:
    Entity( const Entity& );		/* Copying is verbotten */
    Entity& operator=( const Entity& );

public:
    /* Initialization */

    virtual bool check() const = 0;
    virtual Entity& configure( const unsigned );
    virtual unsigned findChildren( Call::stack&, const bool ) const;
    virtual Entity& initWait();
    virtual Entity& initThroughputBound() { return *this; }
    virtual Entity& initPopulation() = 0;
    virtual Entity& initJoinDelay() { return *this; }
    virtual Entity& initThreads() { return *this; }
	
    /* Instance Variable Access */
	   
    virtual LQIO::DOM::Entity * getDOM() const { return _dom; }
    virtual int priority() const { return 0; }
    virtual Entity& priority( const int ) { return *this; }
    virtual scheduling_type scheduling() const { return getDOM()->getSchedulingType(); }
    virtual unsigned copies() const;
    virtual unsigned replicas() const;
    double population() const { return myPopulation; }
    virtual double variance() const { return myVariance; }
    virtual const Processor * getProcessor() const { return 0; }
    unsigned submodel() const { return _submodel; }
    Entity& setSubmodel( const unsigned submodel ) { _submodel = submodel; return *this; }
    virtual double thinkTime( const unsigned = 0, const unsigned = 0 ) const { return myThinkTime; }
    virtual Entity& setOverlapFactor( const double ) { return *this; }

    virtual unsigned int fanOut( const Entity * ) const = 0;
    virtual unsigned int fanIn( const Task * ) const = 0;

    double throughput() const;
    double utilization() const;
    double saturation() const;

    /* Queries */

    virtual bool hasVariance() const { return attributes.variance; }
    bool hasDeterministicPhases() const { return attributes.deterministic; }
    bool hasSecondPhase() const { return (bool)(_maxPhase > 1); }
    bool hasOpenArrivals() const;
    
    virtual unsigned hasClientChain( const unsigned, const unsigned ) const { return 0; }
    unsigned hasServerChain( const unsigned k ) const;
    virtual bool hasActivities() const { return false; }
    bool hasThreads() const { return nThreads() > 1 ? true : false; }
    virtual bool hasSynchs() const { return false; }

    bool isInOpenModel() const { return attributes.open_model ? true : false; }
    Entity& isInOpenModel( const bool yesOrNo ) { attributes.open_model = yesOrNo; return *this; }
    bool isInClosedModel() const { return attributes.closed_model ? true : false; }
    Entity& isInClosedModel( const bool yesOrNo ) { attributes.closed_model = yesOrNo; return *this; }
    Entity& isPureServer( const bool yesOrNo ) { attributes.pure_server = yesOrNo; return *this; }
    bool isPureServer() const { return (bool)attributes.pure_server; }
    Entity& isPureDelay( const bool yesOrNo ) { attributes.pure_delay = yesOrNo; return *this; }
    bool isPureDelay() const { return (bool)attributes.pure_delay; }
    Entity& initialized( const bool yesOrNo ) { attributes.initialized = yesOrNo; return *this; }
    bool initialized() const { return (bool)attributes.initialized; }
    virtual bool isUsed() const { return submodel() > 0; }

    virtual bool isTask() const          { return false; }
    virtual bool isReferenceTask() const { return false; }
    virtual bool isProcessor() const     { return false; }
    virtual bool isInfinite() const;

    bool isCalledBy( const Task* ) const;
    bool isMultiServer() const   	 { return copies() > 1; }

    bool schedulingIsOk( const unsigned bits ) const;

    const std::vector<Entry *>& entries() const { return _entries; }
    Entity& addEntry( Entry * );
    Entry * entryAt( const unsigned index ) const { return _entries.at(index); }

    Entity& addTask( const Task * aTask ) { _tasks.insert( aTask ); return *this; }
    Entity& removeTask( const Task * aTask ) { _tasks.erase( aTask ); return *this; }
    const std::set<const Task *>& tasks() const { return _tasks; }

    virtual const std::string& name() const { return getDOM()->getName(); }
    unsigned maxPhase() const { return _maxPhase; }

    unsigned nEntries() const { return _entries.size(); }
    virtual unsigned nClients() const = 0;
    const Entity& clients( std::set<Task *>& ) const;
    double nCustomers( ) const;

    Entity& addServerChain( const unsigned k ) { _serverChains.push_back(k); return *this; }
    const ChainVector& serverChains() const { return _serverChains; }
    Server * serverStation() const { return myServerStation; }

    bool markovOvertaking() const;

    double openArrivalRate() const;

    /* Computation */
	
    Probability prInterlock( const Task& ) const;

    virtual double prOt( const unsigned, const unsigned, const unsigned ) const { return 0.0; }
    void setIdleTime( const double );
    Entity& computeUtilization();
    virtual Entity& computeVariance();
    void setOvertaking( const unsigned, const std::set<Task *>& );
    virtual Entity& updateWait( const Submodel&, const double ) { return *this; }	/* NOP */
    virtual double updateWaitReplication( const Submodel&, unsigned& ) { return 0.0; }	/* NOP */
    double deltaUtilization() const;

    /* Dynamic Updates / Late Finalization */
    /* In order to integrate LQX's support for model changes we need to have a way  */
    /* of re-calculating what used to be static for all dynamically editable values */
	
    virtual Entity& recalculateDynamicValues() { return *this; }

    /* Sanity Check */

    virtual const Entity& sanityCheck() const;

    /* MVA interface */

    virtual Server * makeServer( const unsigned ) = 0;

    /* Threads */
	
    virtual unsigned nThreads() const { return 1; }		/* Return the number of threads. */
    virtual unsigned concurrentThreads() const { return 1; }	/* Return the number of threads. */
    virtual void joinOverlapFactor( const Submodel&, VectorMath<double>* ) const {};	/* NOP? */
	
    /* Printing */

    virtual ostream& print( ostream& ) const = 0;
    virtual ostream& printJoinDelay( ostream& output ) const { return output; }
    static SRVNManip print_server_chains( const Entity& entity ) { return SRVNManip( output_server_chains, entity ); }
    static SRVNManip print_info( const Entity& entity ) { return SRVNManip( output_entity_info, entity ); }

private:
    static ostream& output_server_chains( ostream& output, const Entity& aServer );
    static ostream& output_entity_info( ostream& output, const Entity& aServer );
    
protected:
    virtual unsigned validScheduling() const;
    virtual scheduling_type defaultScheduling() const { return SCHEDULE_FIFO; }
    double computeIdleTime( const unsigned, const double ) const;
	
private:
    void setServiceTime( Server * station, const Entry * anEntry, unsigned k ) const;

public:
    Interlock * interlock;		/* For interlock calculation.	*/

protected:
    LQIO::DOM::Entity* _dom;		/* The DOM Representation	*/
    std::vector<Entry *> _entries;	/* Entries for this entity.	*/
    std::set<const Task *> _tasks;	/* Clients of this entity	*/
    
    double myPopulation;		/* customers when I'm a client	*/
    double myVariance;			/* Computed variance.		*/
    double myThinkTime;			/* Think time.			*/
    Server * myServerStation;		/* Servers by submodel.		*/

    struct {
	unsigned initialized:1;		/* Task was initialized.	*/
	unsigned closed_model:1;	/* Stn in in closed model.	*/
	unsigned open_model:1;		/* Stn is in open model.	*/
	unsigned deterministic:1;	/* an entry has det. phase.	*/
	unsigned pure_delay:1;		/* Wierd task.			*/
	unsigned pure_server:1;		/* Can use FCFS schedulging.	*/
 	unsigned variance:1;		/* an entry has Cv_sqn != 1.	*/
    } attributes;

private:
    unsigned _submodel;			/* My submodel, 0 == ref task.	*/
    unsigned _maxPhase;			/* Largest phase.		*/
    double _utilization;		/* Utilization			*/
    mutable double _lastUtilization;	/* For convergence test.	*/
    /* MVA interface */

    ChainVector _serverChains;		/* Chains for this server.	*/
};
#endif
