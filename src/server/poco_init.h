#ifndef _POCO_INIT_H_
#define _POCO_INIT_H_

#include "Poco/Net/SSLManager.h"
#include "Poco/Net/Context.h"
#include <Poco/Net/AcceptCertificateHandler.h>
#include "Poco/Net/InvalidCertificateHandler.h"
#include "Poco/SharedPtr.h"

#include <Poco/Logger.h>
#include <Poco/ConsoleChannel.h>
#include <Poco/FileChannel.h>
#include <Poco/AutoPtr.h>
#include <Poco/FormattingChannel.h>
#include <Poco/PatternFormatter.h>

class PocoInit
{
public:
    PocoInit(std::string name)
    {

        // Initialize logger
        // Poco::AutoPtr<Poco::PatternFormatter> formatter(new Poco::PatternFormatter);
        // Poco::AutoPtr<Poco::ConsoleChannel> consoleChannel(new Poco::ConsoleChannel);
        // formatter->setProperty("pattern", "%Y-%m-%d %H:%M:%S [%p] %t");
        // Poco::AutoPtr<Poco::FormattingChannel> formattingChannel(new Poco::FormattingChannel(formatter, consoleChannel));

        // Poco::AutoPtr<Poco::FileChannel> fileChannel(new Poco::FileChannel);
        // fileChannel->setProperty("path", name + ".log");            // Log file name
        // fileChannel->setProperty("purgeCount", "2");            // Keep last 2 archives
        // formatter->setProperty("pattern", "%Y-%m-%d %H:%M:%S [%p] %t");
        // Poco::AutoPtr<Poco::FormattingChannel> formattingChannel(new Poco::FormattingChannel(formatter, fileChannel));

        // Poco::Logger::root().setChannel(formattingChannel);
        // Poco::Logger::root().setLevel("debug");

        // Initialize SSL
        Poco::Net::initializeSSL();

        Poco::SharedPtr<Poco::Net::InvalidCertificateHandler> ptrCert =
                new Poco::Net::AcceptCertificateHandler(false);

        Poco::Net::Context::Ptr context = new Poco::Net::Context(
                Poco::Net::Context::CLIENT_USE,
                "", "", "", // No certs needed for client
                Poco::Net::Context::VERIFY_RELAXED,
                9,                                                                    // Verification depth
                false,                                                            // Load default CA
                "ALL:!ADH:!LOW:!EXP:!MD5:@STRENGTH" // Cipher list
        );

        Poco::Net::SSLManager::instance().initializeClient(nullptr, ptrCert, context);
    }

    ~PocoInit()
    {
        Poco::Net::uninitializeSSL();
    }
};

#endif