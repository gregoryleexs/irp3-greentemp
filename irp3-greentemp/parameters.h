/*        Ubidots topic format 
 publish : /v1.6/devices/clientid
 subscribe : /v1.6/devices/lev2/lev3/lv 
 
 for use with set_topics() in ino
 see https://help.ubidots.com/en/articles/570008-ubidots-mqtt-broker 
 
 for use with client.publish() and client.subscribe()
 see https://pubsubclient.knolleary.net/api
*/

     #define clientid "**************"    //  unique id of this device
     #define lev2 "************"                //  subscribing id of target device
     #define lev3 "Temperature"             //  subscribing variable of target device
                

/****** Others *******************/
     #define keepalive 60          // time in s
     #define mintxinterval 10000   // time in ms
     #define maxtopiclength 150    // set limit for length of topics
