#ifndef __HN_DATASINK_DEVICE_PRIVATE_H__
#define __HN_DATASINK_DEVICE_PRIVATE_H__

#include <string>
#include <vector>

#include "Poco/Util/ServerApplication.h"
#include "Poco/Util/OptionSet.h"

#include <hnode2/HNodeDevice.h>
#include <hnode2/HNodeConfig.h>
#include <hnode2/HNEPLoop.h>
#include <hnode2/HNReqWaitQueue.h>

#define HNODE_DATASINK_DEVTYPE   "hnode2-datasink-device"

typedef enum HNTestDeviceResultEnum
{
  HNDSD_RESULT_SUCCESS,
  HNDSD_RESULT_FAILURE,
  HNDSD_RESULT_BAD_REQUEST,
  HNDSD_RESULT_SERVER_ERROR
}HNDSD_RESULT_T;

class HNDataSinkDevice : public Poco::Util::ServerApplication, public HNDEPDispatchInf //, public HNEPLoopCallbacks
{
    private:
        bool _helpRequested   = false;
        bool _debugLogging    = false;
        bool _instancePresent = false;

        std::string _instance; 
        std::string m_instanceName;

        HNodeDevice m_hnodeDev;

        void displayHelp();

        bool configExists();
        HNDSD_RESULT_T initConfig();
        HNDSD_RESULT_T readConfig();
        HNDSD_RESULT_T updateConfig();

    protected:
        // HNDevice REST callback
        virtual void dispatchEP( HNodeDevice *parent, HNOperationData *opData );

        // Poco funcions
        void defineOptions( Poco::Util::OptionSet& options );
        void handleOption( const std::string& name, const std::string& value );
        int main( const std::vector<std::string>& args );
};

#endif // __HN_DATASINK_DEVICE_PRIVATE_H__
