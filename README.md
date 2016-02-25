# ISMRMRD-CommunicationPrototype
Client and Server prototypes for communicating MR data in the ISMRMRD format using ISMRMRD Communication Protocol

02/25/2016 At initial commit:

           Server was developed to the point where it can accept multiple client connection requests,
           spawn reader and writer threads per client, and echo the received messages. 
           
           Client was previously developed so that the basic communication with the server can be tested. After
           the basic communications have been demonstrated, the Client is being further updated to actually 
           break a .h5 file containing MR data in the ISMRMRD format into frames per Protocol, send the data out,
           and receive the returned by the server data. The source code is currently being updated and does not yet
           compile.
