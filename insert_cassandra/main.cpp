/*
  Copyright (c) 2013 Matthew Stump

  This file is part of cassandra.

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

  http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
*/

#include <cassert>

#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/thread.hpp>
#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/program_options/option.hpp>
#include <boost/program_options/variables_map.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/uuid/uuid.hpp>            // uuid class
#include <boost/uuid/uuid_generators.hpp> // generators
#include <boost/uuid/uuid_io.hpp>         // streaming operators etc.

#include<ctime>

#include <cql/cql.hpp>
#include <cql/cql_error.hpp>
#include <cql/cql_event.hpp>
#include <cql/cql_connection.hpp>
#include <cql/cql_connection_factory.hpp>
#include <cql/cql_session.hpp>
#include <cql/cql_cluster.hpp>
#include <cql/cql_builder.hpp>
#include <cql/cql_execute.hpp>
#include <cql/cql_result.hpp>


const std::string currentDateTime() {
    time_t     now = time(0);
    struct tm  tstruct;
    char       buf[80];
    tstruct = *localtime(&now);
    // Visit http://en.cppreference.com/w/cpp/chrono/c/strftime
    // for more information about date/time format
    strftime(buf, sizeof(buf), "%Y-%m-%d %X", &tstruct);

    return buf;
}


// helper function to print query results
void
print_rows(
    cql::cql_result_t& result)
{
    while (result.next()) {
        for (size_t i = 0; i < result.column_count(); ++i) {
            cql::cql_byte_t* data = NULL;
            cql::cql_int_t size = 0;

            result.get_data(i, &data, size);

            std::cout.write(reinterpret_cast<char*>(data), size);
            for (int i = size; i < 25; i++) {
                std::cout << ' ' ;
            }
            std::cout << " | ";
        }
        std::cout << std::endl;
    }
}


// This function is called asynchronously every time an event is logged
void
log_callback(
    const cql::cql_short_t,
    const std::string& message)
{
    std::cout << "LOG: " << message << std::endl;
}

void
demo(
    const std::string& host,
    bool               use_ssl,
    std::string        file,
    std::string        rpiid,
    std::string        sensor_id,
    std::string        when,
    std::string        ocrvalue)
{
    try
    {
		boost::shared_ptr<cql::cql_builder_t> builder = cql::cql_cluster_t::builder();
		builder->with_log_callback(&log_callback);
        builder->add_contact_point(boost::asio::ip::address::from_string(host));

		if (use_ssl) {
			builder->with_ssl();
		}

		boost::shared_ptr<cql::cql_cluster_t> cluster(builder->build());
		boost::shared_ptr<cql::cql_session_t> session(cluster->connect());

		if (session) {

            // execute a query, switch keyspaces
            boost::shared_ptr<cql::cql_query_t> use_system(
                new cql::cql_query_t("USE meterspace;", cql::CQL_CONSISTENCY_ONE));


            boost::shared_future<cql::cql_future_result_t> future = session->query(use_system);

            // wait for the query to execute
            future.wait();

            // check whether the query succeeded
            std::cout << "switch keyspace successfull? " << (!future.get().error.is_err() ? "true" : "false") << std::endl;

            boost::uuids::uuid uuid = boost::uuids::random_generator()();
            std::stringstream ss;
            ss << uuid;
            
            std::string sql = std::string("INSERT INTO metervalue (date_added, file, rpiid, when, id, val_ocr, val_displayed, sensor_id, valid) VALUES ('" + currentDateTime() + "', '" + file + "', '" + rpiid + "', '" + when + "', " + ss.str() + ", '" + ocrvalue + "', '" + ocrvalue + "', '"  + sensor_id + "', false);");
            // execute a query, select all rows from the keyspace
            boost::shared_ptr<cql::cql_query_t> select(
                new cql::cql_query_t(sql, cql::CQL_CONSISTENCY_ONE));
            future = session->query(select);

            // wait for the query to execute
            future.wait();

            // check whether the query succeeded
            std::cout << "select successfull? " << (!future.get().error.is_err() ? "true" : "false") << std::endl;
            if (future.get().result) {
                // print the rows return by the successful query
                print_rows(*future.get().result);
            }

            // close the connection session
            session->close();
		}

		cluster->shutdown();
        std::cout << "THE END" << std::endl;
    }
    catch (std::exception& e)
    {
        std::cout << "Exception: " << e.what() << std::endl;
    }
}

int
main(
    int    argc,
    char** vargs)
{
    bool ssl = false;
    std::string host;
    std::string file;
    std::string ocrvalue;


    boost::program_options::options_description desc("Options");
    desc.add_options()
        ("help", "produce help message")
        ("file", boost::program_options::value<std::string>(&file)->default_value("default_raspberry"), "input file whose name matches rpiid_raspicam_timestamp.txt and content contains the OCR result of the image")
        ("value", boost::program_options::value<std::string>(&ocrvalue), "value the OCR recognized")
        ("ssl",  boost::program_options::value<bool>(&ssl)->default_value(false), "use SSL")
        ("host", boost::program_options::value<std::string>(&host)->default_value("127.0.0.1"), "node to use as initial contact point");

    boost::program_options::variables_map variables_map;
    try {
        boost::program_options::store(boost::program_options::parse_command_line(argc, vargs, desc), variables_map);
        boost::program_options::notify(variables_map);
    }
    catch (boost::program_options::unknown_option ex) {
        std::cerr << desc << "\n";
        std::cerr << ex.what() << "\n";
        return 1;
    }

    if (variables_map.count("help")) {
        std::cerr << desc << "\n";
        return 0;
    }

    cql::cql_initialize();
    std::cout << "process input file " << file << "\n";
    std::string rpiid, when, sensor_id;
    char delim = '_';
    int index1 = file.find(delim, 0);
    int index2 = file.find(delim, index1 + 1);
    int index3 = file.find(delim, index2 + 1);
    int index4 = file.find('.', index3+1);
    rpiid = file.substr(0, index1);
    sensor_id = file.substr(index1 + 1, index2 - index1 - 1);
    when = file.substr(index2 + 1, index4 - index2 - 1);
    replace(when.begin(), when.end(), 'h',':');
    replace(when.begin(), when.end(), 'm',':');
    replace(when.begin(), when.end(), '_',' ');
    when = when.substr(0, when.size()-1);

    std::cout << index1 << " " << index2 << " " << index3 << " " << index4 << "\n";
    std::cout << "rpiid = " << rpiid << " when = " << when << " sensor_id = " << sensor_id << " ocrvalue = " << ocrvalue << "\n";



    
    demo(host, ssl, file, rpiid, sensor_id, when, ocrvalue);

    cql::cql_terminate();
    return 0;
}
