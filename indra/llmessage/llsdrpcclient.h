/** 
 * @file llsdrpcclient.h
 * @author Phoenix
 * @date 2005-11-05
 * @brief Implementation and helpers for structure data RPC clients.
 *
 * $LicenseInfo:firstyear=2005&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc.
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 * 
 * Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
 * $/LicenseInfo$
 */

#ifndef LL_LLSDRPCCLIENT_H
#define LL_LLSDRPCCLIENT_H

/** 
 * This file declares classes to encapsulate a basic structured data
 * remote procedure client.
 */

#include "llchainio.h"
#include "llfiltersd2xmlrpc.h"
#include "lliopipe.h"
#include "llurlrequest.h"

/** 
 * @class LLSDRPCClientResponse
 * @brief Abstract base class to represent a response from an SD server.
 *
 * This is used as a base class for callbacks generated from an
 * structured data remote procedure call. The
 * <code>extractResponse</code> method will deal with the llsdrpc method
 * call overhead, and keep track of what to call during the next call
 * into <code>process</code>. If you use this as a base class, you
 * need to implement <code>response</code>, <code>fault</code>, and
 * <code>error</code> to do something useful. When in those methods,
 * you can parse and utilize the mReturnValue member data.
 */
class LLSDRPCResponse : public LLIOPipe
{
public:
	LLSDRPCResponse();
	virtual ~LLSDRPCResponse();

	/** 
	 * @brief This method extracts the response out of the sd passed in
	 *
	 * Any appropriate data found in the sd passed in will be
	 * extracted and managed by this object - not copied or cloned. It
	 * will still be up to the caller to delete the pointer passed in.
	 * @param sd The raw structured data response from the remote server.
	 * @return Returns true if this was able to parse the structured data.
	 */
	bool extractResponse(const LLSD& sd);

protected:
	/** 
	 * @brief Method called when the response is ready.
	 */
	virtual bool response(LLPumpIO* pump) = 0;

	/** 
	 * @brief Method called when a fault is generated by the remote server.
	 */
	virtual bool fault(LLPumpIO* pump) = 0;

	/** 
	 * @brief Method called when there was an error
	 */
	virtual bool error(LLPumpIO* pump) = 0;

protected:
	/* @name LLIOPipe virtual implementations
	 */
	//@{
	/** 
	 * @brief Process the data in buffer
	 */
	virtual EStatus process_impl(
		const LLChannelDescriptors& channels,
		buffer_ptr_t& buffer,
		bool& eos,
		LLSD& context,
		LLPumpIO* pump);
	//@}
	
protected:
	LLSD mReturnValue;
	bool mIsError;
	bool mIsFault;
};

/** 
 * @class LLSDRPCClient
 * @brief Client class for a structured data remote procedure call.
 *
 * This class helps deal with making structured data calls to a remote
 * server. You can visualize the calls as:
 * <code>
 * response = uri.method(parameter)
 * </code>
 * where you pass in everything to <code>call</code> and this class
 * takes care of the rest of the details.
 * In typical usage, you will derive a class from this class and
 * provide an API more useful for the specific application at
 * hand. For example, if you were writing a service to send an instant
 * message, you could create an API for it to send the messsage, and
 * that class would do the work of translating it into the method and
 * parameter, find the destination, and invoke <code>call</call> with
 * a useful implementation of LLSDRPCResponse passed in to handle the
 * response from the network.
 */
class LLSDRPCClient : public LLIOPipe
{
public:
	LLSDRPCClient();
	virtual ~LLSDRPCClient();

	/** 
	 * @brief Enumeration for tracking which queue to process the
	 * response.
	 */
	enum EPassBackQueue
	{
		EPBQ_PROCESS,
		EPBQ_CALLBACK,
	};

	/** 
	 * @brief Call a method on a remote LLSDRPCServer
	 *
	 * @param uri The remote object to call, eg,
	 * http://localhost/usher. If you are using a factory with a fixed
	 * url, the uri passed in will probably be ignored.
	 * @param method The method to call on the remote object
	 * @param parameter The parameter to pass into the remote
	 * object. It is up to the caller to delete the value passed in.
	 * @param response The object which gets the response.
	 * @param queue Specifies to call the response on the process or
	 * callback queue.
	 * @return Returns true if this object will be able to make the RPC call.
	 */
	bool call(
		const std::string& uri,
		const std::string& method,
		const LLSD& parameter,
		LLSDRPCResponse* response,
		EPassBackQueue queue);

	/** 
	 * @brief Call a method on a remote LLSDRPCServer
	 *
	 * @param uri The remote object to call, eg,
	 * http://localhost/usher. If you are using a factory with a fixed
	 * url, the uri passed in will probably be ignored.
	 * @param method The method to call on the remote object
	 * @param parameter The seriailized parameter to pass into the
	 * remote object.
	 * @param response The object which gets the response.
	 * @param queue Specifies to call the response on the process or
	 * callback queue.
	 * @return Returns true if this object will be able to make the RPC call.
	 */
	bool call(
		const std::string& uri,
		const std::string& method,
		const std::string& parameter,
		LLSDRPCResponse* response,
		EPassBackQueue queue);

protected:
	/** 
	 * @brief Enumeration for tracking client state.
	 */
	enum EState
	{
		STATE_NONE,
		STATE_READY,
		STATE_WAITING_FOR_RESPONSE,
		STATE_DONE
	};
	
	/* @name LLIOPipe virtual implementations
	 */
	//@{
	/** 
	 * @brief Process the data in buffer
	 */
	virtual EStatus process_impl(
		const LLChannelDescriptors& channels,
		buffer_ptr_t& buffer,
		bool& eos,
		LLSD& context,
		LLPumpIO* pump);
	//@}

protected:
	EState mState;
	std::string mURI;
	std::string mRequest;
	EPassBackQueue mQueue;
	LLIOPipe::ptr_t mResponse;
};

/** 
 * @class LLSDRPCClientFactory
 * @brief Basic implementation for making an SD RPC client factory
 *
 * This class eases construction of a basic sd rpc client. Here is an
 * example of it's use:
 * <code>
 *  class LLUsefulService : public LLService { ... }
 *  LLService::registerCreator(
 *    "useful",
 *    LLService::creator_t(new LLSDRPCClientFactory<LLUsefulService>))
 * </code>
 */
template<class Client>
class LLSDRPCClientFactory : public LLChainIOFactory
{
public:
	LLSDRPCClientFactory() {}
	LLSDRPCClientFactory(const std::string& fixed_url) : mURL(fixed_url) {}
	virtual bool build(LLPumpIO::chain_t& chain, LLSD context) const
	{
		lldebugs << "LLSDRPCClientFactory::build" << llendl;
		LLURLRequest* http(new LLURLRequest(HTTP_POST));
		if(!http->isValid())
		{
			llwarns << "Creating LLURLRequest failed." << llendl ;
			delete http;
			return false;
		}

		LLIOPipe::ptr_t service(new Client);
		chain.push_back(service);		
		LLIOPipe::ptr_t http_pipe(http);
		http->addHeader(HTTP_OUT_HEADER_CONTENT_TYPE, HTTP_CONTENT_TEXT_LLSD);
		if(mURL.empty())
		{
			chain.push_back(LLIOPipe::ptr_t(new LLContextURLExtractor(http)));
		}
		else
		{
			http->setURL(mURL);
		}
		chain.push_back(http_pipe);
		chain.push_back(service);
		return true;
	}
protected:
	std::string mURL;
};

/** 
 * @class LLXMLSDRPCClientFactory
 * @brief Basic implementation for making an XMLRPC to SD RPC client factory
 *
 * This class eases construction of a basic sd rpc client which uses
 * xmlrpc as a serialization grammar. Here is an example of it's use:
 * <code>
 *  class LLUsefulService : public LLService { ... }
 *  LLService::registerCreator(
 *    "useful",
 *    LLService::creator_t(new LLXMLSDRPCClientFactory<LLUsefulService>))
 * </code>
 */
template<class Client>
class LLXMLSDRPCClientFactory : public LLChainIOFactory
{
public:
	LLXMLSDRPCClientFactory() {}
	LLXMLSDRPCClientFactory(const std::string& fixed_url) : mURL(fixed_url) {}
	virtual bool build(LLPumpIO::chain_t& chain, LLSD context) const
	{
		lldebugs << "LLXMLSDRPCClientFactory::build" << llendl;

		LLURLRequest* http(new LLURLRequest(HTTP_POST));
		if(!http->isValid())
		{
			llwarns << "Creating LLURLRequest failed." << llendl ;
			delete http;
			return false ;
		}
		LLIOPipe::ptr_t service(new Client);
		chain.push_back(service);		
		LLIOPipe::ptr_t http_pipe(http);
		http->addHeader(HTTP_OUT_HEADER_CONTENT_TYPE, HTTP_CONTENT_TEXT_XML);
		if(mURL.empty())
		{
			chain.push_back(LLIOPipe::ptr_t(new LLContextURLExtractor(http)));
		}
		else
		{
			http->setURL(mURL);
		}
		chain.push_back(LLIOPipe::ptr_t(new LLFilterSD2XMLRPCRequest(NULL)));
		chain.push_back(http_pipe);
		chain.push_back(LLIOPipe::ptr_t(new LLFilterXMLRPCResponse2LLSD));
		chain.push_back(service);
		return true;
	}
protected:
	std::string mURL;
};

#endif // LL_LLSDRPCCLIENT_H
