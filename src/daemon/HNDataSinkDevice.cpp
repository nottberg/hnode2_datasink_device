#include <unistd.h>
#include <string.h>
#include <syslog.h>

#include <iostream>
#include <sstream>
#include <thread>

#include "Poco/Util/ServerApplication.h"
#include "Poco/Util/Option.h"
#include "Poco/Util/OptionSet.h"
#include "Poco/Util/HelpFormatter.h"
#include "Poco/Checksum.h"
#include <Poco/JSON/Object.h>
#include <Poco/JSON/Parser.h>
#include <Poco/StreamCopier.h>

#include <hnode2/HNodeDevice.h>

#include "HNDataSinkDevicePrivate.h"

using namespace Poco::Util;

namespace pjs = Poco::JSON;
namespace pdy = Poco::Dynamic;

// Forward declaration
extern const std::string g_HNode2DataSinkRest;

void 
HNDataSinkDevice::defineOptions( OptionSet& options )
{
    ServerApplication::defineOptions( options );

    options.addOption(
              Option("help", "h", "display help").required(false).repeatable(false));

    options.addOption(
              Option("debug","d", "Enable debug logging").required(false).repeatable(false));

    options.addOption(
              Option("instance", "", "Specify the instance name of this daemon.").required(false).repeatable(false).argument("name"));

}

void 
HNDataSinkDevice::handleOption( const std::string& name, const std::string& value )
{
    ServerApplication::handleOption( name, value );
    if( "help" == name )
        _helpRequested = true;
    else if( "debug" == name )
        _debugLogging = true;
    else if( "instance" == name )
    {
         _instancePresent = true;
         _instance = value;
    }
}

void 
HNDataSinkDevice::displayHelp()
{
    HelpFormatter helpFormatter(options());
    helpFormatter.setCommand(commandName());
    helpFormatter.setUsage("[options]");
    helpFormatter.setHeader("HNode2 Switch Daemon.");
    helpFormatter.format(std::cout);
}

int 
HNDataSinkDevice::main( const std::vector<std::string>& args )
{
    m_instanceName = "default";
    if( _instancePresent == true )
        m_instanceName = _instance;

    m_hnodeDev.setDeviceType( HNODE_DATASINK_DEVTYPE );
    m_hnodeDev.setInstance( m_instanceName );

    HNDEndpoint hndEP;

    hndEP.setDispatch( "hnode2DataSink", this );
    hndEP.setOpenAPIJson( g_HNode2DataSinkRest ); 

    m_hnodeDev.addEndpoint( hndEP );

    std::cout << "Looking for config file" << std::endl;
    
    if( configExists() == false )
    {
        initConfig();
    }

    readConfig();

    // Start up the hnode device
    m_hnodeDev.start();

    waitForTerminationRequest();

    return Application::EXIT_OK;
}

bool 
HNDataSinkDevice::configExists()
{
    HNodeConfigFile cfgFile;

    return cfgFile.configExists( HNODE_DATASINK_DEVTYPE, m_instanceName );
}

HNDSD_RESULT_T
HNDataSinkDevice::initConfig()
{
    HNodeConfigFile cfgFile;
    HNodeConfig     cfg;

    m_hnodeDev.initConfigSections( cfg );

    cfg.debugPrint(2);
    
    std::cout << "Saving config..." << std::endl;
    if( cfgFile.saveConfig( HNODE_DATASINK_DEVTYPE, m_instanceName, cfg ) != HNC_RESULT_SUCCESS )
    {
        std::cout << "ERROR: Could not save initial configuration." << std::endl;
        return HNDSD_RESULT_FAILURE;
    }

    return HNDSD_RESULT_SUCCESS;
}

HNDSD_RESULT_T
HNDataSinkDevice::readConfig()
{
    HNodeConfigFile cfgFile;
    HNodeConfig     cfg;

    if( configExists() == false )
        return HNDSD_RESULT_FAILURE;

    std::cout << "Loading config..." << std::endl;

    if( cfgFile.loadConfig( HNODE_DATASINK_DEVTYPE, m_instanceName, cfg ) != HNC_RESULT_SUCCESS )
    {
        std::cout << "ERROR: Could not load saved configuration." << std::endl;
        return HNDSD_RESULT_FAILURE;
    }
  
    std::cout << "cl1" << std::endl;
    m_hnodeDev.readConfigSections( cfg );

    std::cout << "Config loaded" << std::endl;

    return HNDSD_RESULT_SUCCESS;
}

HNDSD_RESULT_T
HNDataSinkDevice::updateConfig()
{
    HNodeConfigFile cfgFile;
    HNodeConfig     cfg;

    cfg.debugPrint(2);
    
    std::cout << "Saving config..." << std::endl;
    if( cfgFile.saveConfig( HNODE_DATASINK_DEVTYPE, m_instanceName, cfg ) != HNC_RESULT_SUCCESS )
    {
        std::cout << "ERROR: Could not save configuration." << std::endl;
        return HNDSD_RESULT_FAILURE;
    }
    std::cout << "Config saved" << std::endl;

    return HNDSD_RESULT_SUCCESS;
}

void 
HNDataSinkDevice::dispatchEP( HNodeDevice *parent, HNOperationData *opData )
{
    std::cout << "HNDataSinkDevice::dispatchEP() - entry" << std::endl;
    std::cout << "  dispatchID: " << opData->getDispatchID() << std::endl;
    std::cout << "  opID: " << opData->getOpID() << std::endl;
    std::cout << "  thread: " << std::this_thread::get_id() << std::endl;

    std::string opID = opData->getOpID();
          
    // GET "/hnode2/datasink/status"
    if( "getStatus" == opID )
    {
        std::cout << "=== Get Status Request ===" << std::endl;
    
        // Set response content type
        opData->responseSetChunkedTransferEncoding( true );
        opData->responseSetContentType( "application/json" );

        // Create a json status object
        pjs::Object jsRoot;
        jsRoot.set( "overallStatus", "OK" );

        // Render response content
        std::ostream& ostr = opData->responseSend();
        try{ 
            pjs::Stringifier::stringify( jsRoot, ostr, 1 ); 
        } catch( ... ) {
            std::cout << "ERROR: Exception while serializing comment" << std::endl;
        }
            
        // Request was successful
        opData->responseSetStatusAndReason( HNR_HTTP_OK );
    }
    // GET "/hnode2/datasink/logging"
    else if( "getLoggingStatus" == opID )
    {
        std::cout << "=== Get Logging Status ===" << std::endl;

        // Set response content type
        opData->responseSetChunkedTransferEncoding( true );
        opData->responseSetContentType( "application/json" );

        // Create a json root object
        pjs::Array jsRoot;
/*
        pjs::Object w1Obj;
        w1Obj.set( "id", "w1" );
        w1Obj.set( "color", "red" );
        jsRoot.add( w1Obj );

        pjs::Object w2Obj;
        w2Obj.set( "id", "w2" );
        w2Obj.set( "color", "green" );
        jsRoot.add( w2Obj );

        pjs::Object w3Obj;
        w3Obj.set( "id", "w3" );
        w3Obj.set( "color", "blue" );
        jsRoot.add( w3Obj );
*/          
        // Render response content
        std::ostream& ostr = opData->responseSend();
        try{ 
            pjs::Stringifier::stringify( jsRoot, ostr, 1 ); 
        } catch( ... ) {
            std::cout << "ERROR: Exception while serializing comment" << std::endl;
        }
            
        // Request was successful
        opData->responseSetStatusAndReason( HNR_HTTP_OK );
    }
    // PUT "/hnode2/datasink/logging"
    else if( "setLoggingConfig" == opID )
    {
        std::cout << "=== Set Logging Config ===" << std::endl;

        std::istream& rs = opData->requestBody();
        std::string body;
        Poco::StreamCopier::copyToString( rs, body );
        
        //std::cout << "=== Update Widget Put Data (id: " << widgetID << ") ===" << std::endl;
        //std::cout << body << std::endl;

        // Request was successful
        opData->responseSetStatusAndReason( HNR_HTTP_OK );

    }
    // PUT "/hnode2/datasink/logging/log"
    else if( "getLogEntries" == opID )
    {
        std::cout << "=== get Log Entries ===" << std::endl;

/*
        // Set response content type
        opData->responseSetChunkedTransferEncoding( true );
        opData->responseSetContentType( "application/json" );
        
        // Create a json root object
        pjs::Array jsRoot;

        pjs::Object w1Obj;
        w1Obj.set( "id", widgetID );
        w1Obj.set( "color", "black" );
        jsRoot.add( w1Obj );
          
        // Render response content
        std::ostream& ostr = opData->responseSend();
        try{ 
            pjs::Stringifier::stringify( jsRoot, ostr, 1 ); 
        } catch( ... ) {
            std::cout << "ERROR: Exception while serializing comment" << std::endl;
        }
*/            
        // Request was successful
        opData->responseSetStatusAndReason( HNR_HTTP_OK );

    }    
    // POST "/hnode2/datasink/logging/log"
    else if( "addLogEntries" == opID )
    {
        std::istream& rs = opData->requestBody();
        std::string body;
        Poco::StreamCopier::copyToString( rs, body );
        
        std::cout << "=== Add Log Entries Post Data ===" << std::endl;
        std::cout << body << std::endl;

        // Object was created return info
        opData->responseSetCreated( "w1" );
        opData->responseSetStatusAndReason( HNR_HTTP_CREATED );
    }
    else
    {
        // Send back not implemented
        opData->responseSetStatusAndReason( HNR_HTTP_NOT_IMPLEMENTED );
        opData->responseSend();
        return;
    }

    // Return to caller
    opData->responseSend();
}

const std::string g_HNode2DataSinkRest = R"(
{
  "openapi": "3.0.0",
  "info": {
    "description": "",
    "version": "1.0.0",
    "title": ""
  },
  "paths": {
      "/hnode2/datasink/status": {
        "get": {
          "summary": "Get data sink device status.",
          "operationId": "getStatus",
          "responses": {
            "200": {
              "description": "successful operation",
              "content": {
                "application/json": {
                  "schema": {
                    "type": "array"
                  }
                }
              }
            },
            "400": {
              "description": "Invalid status value"
            }
          }
        }
      },

      "/hnode2/datasink/logging": {
        "get": {
          "summary": "Return logging status info.",
          "operationId": "getLoggingStatus",
          "responses": {
            "200": {
              "description": "successful operation",
              "content": {
                "application/json": {
                  "schema": {
                    "type": "object"
                  }
                }
              }
            },
            "400": {
              "description": "Invalid status value"
            }
          }
        },

        "put": {
          "summary": "Set logging configuration parameters",
          "operationId": "setLoggingConfig",
          "responses": {
            "200": {
              "description": "successful operation",
              "content": {
                "application/json": {
                  "schema": {
                    "type": "object"
                  }
                }
              }
            },
            "400": {
              "description": "Invalid status value"
            }
          }
        }
      },

      "/hnode2/datasink/logging/log": {
        "get": {
          "summary": "Return logged data entries.",
          "operationId": "getLogEntries",
          "responses": {
            "200": {
              "description": "successful operation",
              "content": {
                "application/json": {
                  "schema": {
                    "type": "object"
                  }
                }
              }
            },
            "400": {
              "description": "Invalid status value"
            }
          }
        },

        "post": {
          "summary": "Add new log entries",
          "operationId": "addLogEntries",
          "responses": {
            "200": {
              "description": "successful operation",
              "content": {
                "application/json": {
                  "schema": {
                    "type": "object"
                  }
                }
              }
            },
            "400": {
              "description": "Invalid status value"
            }
          }
        }
      }
    }
}
)";

