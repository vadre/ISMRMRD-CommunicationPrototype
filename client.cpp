// client.cpp

#include <iostream>
#include <boost/array.hpp>
#include <boost/asio.hpp>
#include <boost/program_options.hpp>
#include <thread>
#include <ismrmrd_entity.hpp>

using boost::asio::ip::tcp;
namespace po = boost::program_options;

const int max_length = 65536;
//const int max_length = 1024;

/*******************************************************************************
 ******************************************************************************/
void socket_read (tcp::socket *socket)
{
  std::cout << "\nClient Reader started\n" << std::flush;
  std::string data;
  std::string temp;
  size_t temp_actual_len = 0;
  size_t temp_decl_len  = 0;
  boost::system::error_code error;
  std::queue<std::string> dq;
  ISMRMRD_Entity ent_hdr;
  struct timespec ts;
  ts.tv_sec  =    0;
  ts.tv_nsec = 1000;

  while (1)
  {
    //if (!socket->available())
    //{
      //nanosleep (&ts, NULL);
      //continue;
    //}

    size_t len = socket->read_some (boost::asio::buffer (data), error);

    if (error == boost::asio::error::eof)
    {
      std::cout << "\nClient received EOF - closing the socket\n" << std::flush;
      socket->close();
      break; // Connection closed cleanly by peer.
    }
    else if (error)
    {
      std::cout << "\nClient received error: " << error << "\n" << std::flush;
      throw boost::system::system_error (error); // Some other error.
    }

    std::cout << "\nClient Received: " << len << " bytes" << std::endl;
    size_t copy_size = temp_decl_size - temp_actual_size;
    if (copy_size < 0)
    {
      std::cout << "Size Error: decl = " << temp_decl_size << ", actual = " << temp_actual_size << std::endl;
    }
    else if (copy_size == 0)
    {
      // This is either the very first transmission, or the previous transmission resulted in a compete frame
      // with no missing or extra bytes
      copy_size = (uint64_t) data.c_str();
      if (copy_size > len)
      {
        copy_size = len;
      }

      dq.push (data.substr (0, copy_size));
    }
    else
    {
      copy_size =;
    }
    
    if (len <= 0)
    {
      std::cout << "\nReceived 0 bytes transmission - closing connection and processing the data" << std::endl;
      socket->close();
      break;
    }


    uint32_t entity_type  = (uint32_t) data.c_str() + 128;
    uint32_t command_type = (uint32_t) data.c_str() + 192;
    std::cout << "Entity type = " << entity_type << ", command type = " << command_type << std::endl;

    if (entity_type == ISMRMRD_COMMAND && command_type == ISMRMRD_DONE)
    {
        std::cout << "Received DONE command, closing connection and processing the data" << std::endl;
        socket->close();
        break;
    }
  }

  std::cout << "\nFinished processing the data - exiting" << std::endl;
  return;
}

/*******************************************************************************
 ******************************************************************************/
void prepare_data_queue (std::string fname,
                        std::string dname,
                        std::queue<std::string>& fq,
                        std::string client_name,
                        std::atomic<bool>& done)
{
  struct timeval tv;

  // Handshake:
  gettimeofday(&tv, NULL);
  ISMRMRD_Entity ent_hdr (2, ISMRMRD_HANDSHAKE, ISMRMRD_CHAR, 65536);
  ISMRMRD_HANDSHAKE handshake ((uint64_t)(tv.tv_sec), client_name.c_str);
  ISMRMRD_Entity ent_hdr (2, ISMRMRD_XML_HEADER, ISMRMRD_CHAR, 1);

  uint64_t  size = (uint64_t) (sizeof (ISMRMRD_Entity) + sizeof (ISMRMRD_HANDSHAKE));
  std::string data = (char*)&size;
  data += (char*)&ent_hdr;
  data += (char*)&handshake;
  fq.push (data);

  //char     *handshake_data = (char*) malloc (size + sizeof (uint64_t)) ;
  //memset (handshake_data, 0, size + sizeof (uint64_t));
  //strncpy (handshake_data, (char*)&size, sizeof (uint64_t));
  //strncpy (handshake_data + sizeof (uint64_t),
           //(char*)&ent_hdr, sizeof (ISMRMRD_Entity));
  //strncpy (handshake_data + sizeof (uint64_t) + sizeof (ISMRMRD_Entity),
           //(char*)&handshake, sizeof (ISMRMRD_Handshake));
  //fq.push (handshake_data);
  
  // XML Header
  ISMRMRD::Dataset d (fname.c_str(), dname.c_str(), false);
  std::string xml;
  d.readHeader (xml);
  ent_hdr (2, ISMRMRD_XML_HEADER, ISMRMRD_CHAR, 1);
  size  = sizeof (ISMRMRD_Entity) + xml.size();
  data  = (char*)&size;
  data += (char*)&ent_hdr;
  data += (char*)xml.c_str();
  fq.push (data);

  //char *xml_head_data = (char*) malloc (size + sizeof (uint64_t));
  //memset (xml_head_data, 0, size + sizeof (uint64_t));
  //strncpy (xml_head_data, (char*)&size, sizeof (uint64_t));
  //strncpy (xml_head_data + sizeof (uint64_t),
           //(char*)&ent_hdr, sizeof (ISMRMRD_Entity));
  //strncpy (xml_head_data + sizeof (uint64_t) + sizeof (ISMRMRD_Entity),
           //(char*)xml.c_str(), xml.size());
  //fq.push (xml_head_data);

  // Acqisitions
  ISMRMRD::Acquisition acq;
  int num_acq = d.getNumberOfAcquisitions();
  ISMRMRD::IsmrmrdHeader hdr;
  ISMRMRD::deserialize(xml.c_str(),hdr);
  //int acq_length = hdr.encoding[0].encodedSpace.matrixSize.x;
  for (int ii = 0; ii < num_acq; ++ii)
  {
    d.readAcquisition(ii, acq);
    int traj_length = acq.getNumberOfTrajElements();
    int data_length = acq.getNumberOfDataElements();
    int num_coils = acq.active_channels();
    ent_hdr (2, ISMRMRD_MRACQUISITION, ISMRMRD_CXFLOAT, 1);
    size = sizeof (ISMRMRD_Entity) + sizeof (ISMRMRD_AcquisitionHeader) +
           traj_length * sizeof (float) + data_length * sizeof (complex_float);
    data  = (char*)&size;
    data += (char*)&ent_hdr;
    data += (char*)&acq.acq
    fq.push (data);

    //char *acq_data = (char*) malloc (size + sizeof (uint64_t));
    //memset (acq_data, 0, size + sizeof (uint64_t));
    //strncpy (acq_data, (char*) &size, sizeof (uint64_t));
    //strncpy (acq_data + sizeof (uint64_t), (char*) &ent_hdr, sizeof (ISMRMRD_Entity));
    //strncpy (acq_data + sizeof (uint64_t) + sizeof (ISMRMRD_Entity),
             //(char*)acq.acq, size - sizeof (ISMRMRD_Entity));
    //fq.push (acq_data);
  }

  done = true;
}
/*******************************************************************************
 ******************************************************************************/
int main (int argc, char* argv[])
{
  std::string       client_name = "HAL 9000";
  std::string       host        = "127.0.0.1";
  std::string       in_fname    = "FileIn.h5";
  std::string       in_dset     = "/dataset";
  std::string       out_fname   = "FileOut.h5";
  std::string       out_dset    = "/dataset";
  unsigned short    port        = 50050;
  struct timespec   ts;

  ts.tv_sec  =    0;
  ts.tv_nsec = 1000;
  

  po::options_description desc("Allowed options");
  desc.add_options()
    ("help,h", "produce help message")
    ("client name,c", po::value<std::string>(&client_name)->default_value(client_name), "Client Name")
    ("host,H", po::value<std::string>(&host)->default_value(host), "Server IP address")
    ("port,p", po::value<unsigned short>(&port)->default_value(port), "TCP port for server")
    ("fin,i", po::value<std::string>(&in_fname)->default_value(in_fname), "HDF5 Input file")
    ("in_group,I", po::value<std::string>(&in_dset)->default_value(in_dset), "Input group name")
    ("fout,o", po::value<std::string>(&out_fname)->default_value(out_fname), "Output file name")
    ("out_group,O", po::value<std::string>(&out_dset)->default_value(out_dset), "Output group name")
  ;

  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, desc), vm);
  po::notify(vm);

  if (vm.count("help"))
  {
    std::cout << desc << "\n";
    return 0;
  }

  std::cout << "Using client name [" << client_name << "], host IP address [" << host <<
               "], and port [" << port << "]" << std::endl;
  std::cout << "To change re-start with: icli -h" << std::endl;


  // Spawn a thread to prepare the data to be sent to the server
  std::cout << "Attempting to prepare data from " << in_fname << std::endl;
  std::atomic<bool> done (false);
  std::queue<std::string> frame_queue;
  std::thread t1 (prepare_data_queue, in_fname, in_dset, frame_queue, client_name, &done);

  try
  {
    boost::asio::io_service io_service;
    tcp::socket socket(io_service);
    tcp::endpoint endpoint(boost::asio::ip::address::from_string(host), port);

    boost::system::error_code error = boost::asio::error::host_not_found;
    socket.connect(endpoint, error);
    if (error) throw boost::system::system_error(error);

    std::thread t2 (socket_read, &socket);
    sleep(1);

    while (!done || !frame_queue.empty())
    {
      if (frame_queue.empty())
      {
        nanosleep (ts, NULL);
        continue;
      }

      std::cout << client_name << " sends out a frame" << std::endl;
      std::string data = frame_queue.front();
      frame_queue.pop();
      boost::asio::write (socket, boost::asio::buffer (data, data.size()));
    }

    t1.join();
    t2.join();
  }
  catch (std::exception& e)
  {
    std::cerr << e.what() << std::endl;
  }

  return 0;
}
