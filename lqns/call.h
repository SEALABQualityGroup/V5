/* -*- c++ -*-
 * $HeadURL: http://rads-svn.sce.carleton.ca:8080/svn/lqn/trunk-V5/lqns/call.h $
 *
 * Everything you wanted to know about an entry, but were afraid to ask.
 *
 * Copyright the Real-Time and Distributed Systems Group,
 * Department of Systems and Computer Engineering,
 * Carleton University, Ottawa, Ontario, Canada. K1S 5B6
 *
 * November, 1994
 * March, 2004
 *
 * $Id: call.h 13705 2020-07-20 21:46:53Z greg $
 *
 * ------------------------------------------------------------------------
 */

#if	!defined(LQNS_CALL_H)
#define LQNS_CALL_H

#include "dim.h"
#include <cstdlib>
#include <lqio/input.h>
#include <lqio/dom_call.h>
#include <deque>
#include "prob.h"


class Entry;
class Call;
class Entry;
class Entity;
class Submodel;
class Phase;
class Path;
class EntryPath;
class InterlockInfo;
class PathNode;
class Server;
class Task;

typedef void (Call::*callFunc)( const unsigned, const unsigned, const double );
typedef bool (Call::*queryFunc)() const;

/* ------------------- Arcs between entries are... -------------------- */

class Call {

public:
    class Create {
    public:
	Create( Entry * src, unsigned p ) : _src(src), _p(p) {}

	void operator()( const LQIO::DOM::Call * );

    private:
	Entry * _src;
	const unsigned _p;
    };
    
    
    struct Find {
	Find( const Call * call, const bool direct_path ) : _call(call), _direct_path(direct_path), _broken(false) {}

	bool operator()( const Call * ) const;
	
    private:
	const Call * _call;
	const bool _direct_path;
	mutable bool _broken;
    };
	
    class stack : public std::deque<const Call *>
    {
    public:
	stack() : std::deque<const Call *>() {}

	unsigned depth() const;
    };

    class call_cycle
    {
    public:
	call_cycle() {}
	virtual ~call_cycle() throw() {}
    };


    typedef enum { RENDEZVOUS_CALL=0x01, SEND_NO_REPLY_CALL=0x02, FORWARDED_CALL=0x04, OVERTAKING_CALL=0x08 } call_type;

    Call( const Phase * fromPhase, const Entry * toEntry );
    virtual ~Call();

private:
    Call( const Call& ) { abort(); }		/* Copying is verbotten */
    Call& operator=( const Call& ) { abort(); return *this; }

public:
    int operator==( const Call& item ) const;
    int operator!=( const Call& item ) const;

    virtual Call& initWait() = 0;
    bool check() const;

    /* Instance variable access */

    virtual double rendezvous() const;
    virtual Call& rendezvous( const LQIO::DOM::Call* dom ) { _dom = dom; return *this; }
    Call& accumulateRendezvous( const double value )  { abort(); /*myRendezvous += value;*/ return *this; }
    double sendNoReply() const;
    Call& sendNoReply( const LQIO::DOM::Call* dom ) { _dom = dom; return *this; }
    Call& accumulateSendNoReply( const double value ) { abort(); /*mySendNoReply += value;*/ return *this; }
    double forward() const;
    Call& forward( const LQIO::DOM::Call* dom ) { _dom = dom; return *this; }
    unsigned fanOut() const;

    double sumOfCalls() const { return getDOM() ? getDOM()->getCallMeanValue() : 0.0; }


    /* Queries */

    virtual bool isForwardedCall() const { return false; }
    virtual bool isActivityCall() const { return false; }
    virtual bool isProcessorCall() const { return false; }
    bool hasRendezvous() const { return getDOM() ? (getDOM()->getCallType() == LQIO::DOM::Call::RENDEZVOUS || getDOM()->getCallType() == LQIO::DOM::Call::QUASI_RENDEZVOUS) && getDOM()->getCallMeanValue() > 0: false; }
    bool hasSendNoReply() const { return getDOM() ? getDOM()->getCallType() == LQIO::DOM::Call::SEND_NO_REPLY && getDOM()->getCallMeanValue() > 0 : false; }
    bool hasForwarding() const { return  getDOM() ? getDOM()->getCallType() == LQIO::DOM::Call::FORWARD && getDOM()->getCallMeanValue() > 0 : false; }
    bool hasOvertaking() const;
    bool hasNoCall() const { return !hasRendezvous() && !hasSendNoReply() && !hasForwarding(); }
    bool hasRendezvousOrNone() const { return !hasSendNoReply() && !hasForwarding(); }
    bool hasSendNoReplyOrNone() const { return !hasRendezvous() && !hasForwarding(); }
    bool hasForwardingOrNone() const { return !hasRendezvous() && !hasSendNoReply(); }
    bool hasNoForwarding() const { return dstEntry() == nullptr || hasRendezvous() || hasSendNoReply(); }		/* Special case for topological sort */

    virtual const Entry * srcEntry() const;
    virtual const std::string& srcName() const;
    const Phase * srcPhase() const { return source; }
    virtual const Task * srcTask() const;

    const std::string& dstName() const;
    const Entry * dstEntry() const { return destination; }
    unsigned submodel() const;

    double rendezvousDelay() const;
    double rendezvousDelay( const unsigned k );
    double wait() const { return myWait; }
    double elapsedTime() const;
    double queueingTime() const;
    virtual const Call& insertDOMResults() const;
    double nrFactor( const Submodel& aSubmodel, const unsigned k ) const;

    /* Computation */

    double variance() const;
    double CV_sqr() const;
    unsigned followInterlock( std::deque<const Entry *>&, const InterlockInfo&, const unsigned ) const;

    /* MVA interface */

    unsigned getChain() const;
    void setChain( const unsigned k, const unsigned p, const double rate );

    void setVisits( const unsigned k, const unsigned p, const double rate );
    virtual void setLambda( const unsigned k, const unsigned p, const double rate );
    void clearWait( const unsigned k, const unsigned p, const double );
    void saveWait( const unsigned k, const unsigned p, const double );
    void saveOpen( const unsigned k, const unsigned p, const double );

    /* Proxies */

    const Entity * dstTask() const;
    short index() const;
    double serviceTime() const;

public:

    /* There is now only one, and it has a type built in */
    const LQIO::DOM::Call* getDOM() const { return _dom; }

protected:
    const Phase* source;		/* Calling entry.		*/
    double myWait;			/* Waiting time.		*/
    double myQueueLength;

private:
    const Entry* destination;		/* to whom I am referring to	*/

    /* Input */
    const LQIO::DOM::Call* _dom;

    unsigned chainNumber;
};


/* -------------------------------------------------------------------- */

class NullCall : public Call {
public:
    NullCall() : Call(nullptr,nullptr) {}

    virtual NullCall& initWait() { return *this; }
};

/* -------------------------------------------------------------------- */

class TaskCall : public Call {
public:
    TaskCall( const Phase *, const Entry * toEntry );

    virtual TaskCall& initWait();
};


class ForwardedCall : public TaskCall {
public:
    ForwardedCall( const Phase *, const Entry * toEntry, const Call * fwdCall );

    virtual bool isForwardedCall() const { return true; }
    virtual const std::string& srcName() const;
    virtual const ForwardedCall& insertDOMResults() const;

private:
    const Call * myFwdCall;		// Original forwarded call
};

class ProcessorCall : public Call {
public:
    ProcessorCall( const Phase *, const Entry * toEntry );

    virtual bool isProcessorCall() const { return true; }
    virtual ProcessorCall& initWait();
    virtual void setWait(double newWait);
};


class ActivityCall : public TaskCall {
public:
    ActivityCall( const Phase * fromActivity, const Entry * toEntry );

    virtual bool isActivityCall() const { return true; }
    virtual const Entry * srcEntry() const;
    virtual const std::string& srcName() const;
    virtual const Task * srcTask() const;
};


class ActivityForwardedCall : public ActivityCall {
public:
    ActivityForwardedCall( const Phase *, const Entry * toEntry );

    virtual bool isForwardedCall() const { return true; }
};


class ActProcCall : public ProcessorCall {
public:
    ActProcCall( const Phase *, const Entry * toEntry );

    virtual bool isActivityCall() const { return true; }
    virtual const Entry * srcEntry() const;
    virtual const std::string& srcName() const;
    virtual const Task * srcTask() const;
};
#endif
