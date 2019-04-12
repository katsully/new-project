#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"

#include "cinder/Log.h"
#include "cinder/Timeline.h"
#include "cinder/osc/Osc.h"

using namespace ci;
using namespace ci::app;
using namespace std;

#define USE_UDP 1

#if USE_UDP
using Receiver = osc::ReceiverUdp;
using protocol = asio::ip::udp;
#else
using Receiver = osc::ReceiverTcp;
using protocol = asio::ip::tcp;
#endif

const uint16_t localPort = 8000;

class NewProjectApp : public App {
public:
    NewProjectApp();
    void setup() override;
    void draw() override;
    std::vector<float> breakUp(string temp);
    
    
    ivec2   mCurrentCirclePos;
    vec2    mCurrentSquarePos;
    bool    mMouseDown = false;
    
    Receiver mReceiver;
    std::map<uint64_t, protocol::endpoint> mConnections;
    
    vec3 circle;
    std::vector<float> leftLeg;
    std::vector<float> chest;
    std::vector<float> rightLeg;
    std::vector<float> rightWrist;
    std::vector<float> leftWrist;
    std::vector<float> head;
};

NewProjectApp::NewProjectApp()
: mReceiver( localPort )
{
}

void NewProjectApp::setup()
{
    mReceiver.setListener( "/notch/LeftLowerLeg/all",
                          [&]( const osc::Message &msg ){
                              // split up xyx pos and rot values
                              leftLeg = breakUp(msg[0].string());
//                              cout << myVect.at(0) << " " <<  myVect.at(1) <<  " " <<  myVect.at(2) << endl;
//                              circle = vec3(stof(msg[0].string()), stof(msg[1].string()), stof(msg[2].string()));
                          });
    mReceiver.setListener( "/notch/ChestBottom/all",
                          [&]( const osc::Message &msg ){
                              chest = breakUp(msg[0].string());
                          });
    mReceiver.setListener( "/notch/RightLowerLeg/all",
                          [&]( const osc::Message &msg ){
                              rightLeg = breakUp(msg[0].string());
                          });
    mReceiver.setListener( "/notch/RightForeArm/all",
                          [&]( const osc::Message &msg ){
                              rightWrist = breakUp(msg[0].string());
                          });
    mReceiver.setListener( "/notch/LeftForeArm/all",
                          [&]( const osc::Message &msg ){
                              leftWrist = breakUp(msg[0].string());
                          });
    mReceiver.setListener( "/notch/Head/all",
                          [&]( const osc::Message &msg ){
                              head = breakUp(msg[0].string());
                          });
    try {
        // Bind the receiver to the endpoint. This function may throw.
        mReceiver.bind();
    }
    catch( const osc::Exception &ex ) {
        CI_LOG_E( "Error binding: " << ex.what() << " val: " << ex.value() );
        quit();
    }
    
#if USE_UDP
    // UDP opens the socket and "listens" accepting any message from any endpoint. The listen
    // function takes an error handler for the underlying socket. Any errors that would
    // call this function are because of problems with the socket or with the remote message.
    mReceiver.listen(
                     []( asio::error_code error, protocol::endpoint endpoint ) -> bool {
                         if( error ) {
                             CI_LOG_E( "Error Listening: " << error.message() << " val: " << error.value() << " endpoint: " << endpoint );
                             return false;
                         }
                         else
                             return true;
                     });
#else
    mReceiver.setConnectionErrorFn(
                                   // Error Function for Accepted Socket Errors. Will be called anytime there's an
                                   // error reading from a connected socket (a socket that has been accepted below).
                                   [&]( asio::error_code error, uint64_t identifier ) {
                                       if ( error ) {
                                           auto foundIt = mConnections.find( identifier );
                                           if( foundIt != mConnections.end() ) {
                                               // EOF or end of file error isn't specifically an error. It's just that the
                                               // other side closed the connection while you were expecting to still read.
                                               if( error == asio::error::eof ) {
                                                   CI_LOG_W( "Other side closed the connection: " << error.message() << " val: " << error.value() << " endpoint: " << foundIt->second.address().to_string()
                                                            << " port: " << foundIt->second.port() );
                                               }
                                               else {
                                                   CI_LOG_E( "Error Reading from Socket: " << error.message() << " val: "
                                                            << error.value() << " endpoint: " << foundIt->second.address().to_string()
                                                            << " port: " << foundIt->second.port() );
                                               }
                                               mConnections.erase( foundIt );
                                           }
                                       }
                                   });
    auto expectedOriginator = protocol::endpoint( asio::ip::address::from_string( "127.0.0.1" ), 10000 );
    mReceiver.accept(
                     // Error Handler for the acceptor. You'll return true if you want to continue accepting
                     // or fals otherwise.
                     []( asio::error_code error, protocol::endpoint endpoint ) -> bool {
                         if( error ) {
                             CI_LOG_E( "Error Accepting: " << error.message() << " val: " << error.value()
                                      << " endpoint: " << endpoint.address().to_string() );
                             return false;
                         }
                         else
                             return true;
                     },
                     // Accept Handler. Return whether or not the acceptor should cache this connection
                     // (true) or dismiss it (false).
                     [&, expectedOriginator]( osc::TcpSocketRef socket, uint64_t identifier ) -> bool {
                         // Here we return whether or not the remote endpoint is the expected endpoint
                         mConnections.emplace( identifier, socket->remote_endpoint() );
                         return socket->remote_endpoint() == expectedOriginator;
                     } );
#endif
}


void NewProjectApp::draw()
{
    gl::clear( GL_COLOR_BUFFER_BIT );
    gl::setMatricesWindow( getWindowSize() );

    
    // left ankle - RED
    if(leftLeg.size() > 0) {
        gl::color(Color(1,0,0));
        gl::drawSolidEllipse(vec2(leftLeg.at(0)+getWindowWidth()/2, getWindowHeight()-(leftLeg.at(1)+getWindowHeight()/2)), 20, 20);
        gl::drawLine(vec2(0,200), vec2(vec2(leftLeg.at(0)+getWindowWidth()/2, getWindowHeight()-(leftLeg.at(1)+getWindowHeight()/2))));
    }
    // chest - GREEN
    if(chest.size() > 0) {
        gl::color(Color(0,1,0));
        gl::drawSolidEllipse(vec2(chest.at(0)+getWindowWidth()/2, getWindowHeight()-(chest.at(1)+getWindowHeight()/2)), 20, 20);
        gl::drawLine(vec2(getWindowWidth(), 600), vec2(chest.at(0)+getWindowWidth()/2, getWindowHeight()-(chest.at(1)+getWindowHeight()/2)));
    }
    // left arm - BLUE
    if(leftWrist.size() > 0) {
        gl::color(Color(0,0,1));
        gl::drawSolidEllipse(vec2(leftWrist.at(0)+getWindowWidth()/2, getWindowHeight()-(leftWrist.at(1)+getWindowHeight()/2)), 20, 20);
        gl::drawLine(vec2(getWindowWidth(), 400), vec2(leftWrist.at(0)+getWindowWidth()/2, getWindowHeight()-(leftWrist.at(1)+getWindowHeight()/2)));
    }
    // right arm - YELLOW
    if(rightWrist.size() > 0) {
        gl::color(1,1,0);
        gl::drawSolidEllipse(vec2(rightWrist.at(0)+getWindowWidth()/2, getWindowHeight()-(rightWrist.at(1)+getWindowHeight()/2)), 20, 20);
        gl::drawLine(vec2(0,0),vec2(rightWrist.at(0)+getWindowWidth()/2, getWindowHeight()-(rightWrist.at(1)+getWindowHeight()/2)));
    }
    // right Leg - PINK
    if(rightLeg.size() > 0) {
        gl::color(1,0,1);
        gl::drawSolidEllipse(vec2(rightLeg.at(0)+getWindowWidth()/2, getWindowHeight()-(rightLeg.at(1)+getWindowHeight()/2)), 20, 20);
        gl::drawLine(vec2(0,getWindowHeight()),vec2(rightLeg.at(0)+getWindowWidth()/2, getWindowHeight()-(rightLeg.at(1)+getWindowHeight()/2)));
    }
    // head - TEAL
    if(head.size() > 0) {
        gl::color(0,1,1);
        gl::drawSolidEllipse(vec2(head.at(0)+getWindowWidth()/2, getWindowHeight()-(head.at(1)+getWindowHeight()/2)), 20, 20);
        gl::drawLine(vec2(0,300), vec2(head.at(0)+getWindowWidth()/2, getWindowHeight()-(head.at(1)+getWindowHeight()/2)));
    }
}

std::vector<float> NewProjectApp::breakUp(string temp) {
    std::vector<float> vect;
    std::string delimiter = ",";
    size_t pos = 0;
    std::string token;
    while ((pos = temp.find(delimiter)) != std::string::npos) {
        token = temp.substr(0, pos);
        vect.push_back(stof(token));
        temp.erase(0, pos + delimiter.length());
    }
    return vect;
}

CINDER_APP( NewProjectApp, RendererGl )
