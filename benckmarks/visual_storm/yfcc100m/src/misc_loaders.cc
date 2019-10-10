#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <sstream>
#include <iostream>
#include <fstream>
#include <vector>
#include <stdlib.h>     /* system, NULL, EXIT_FAILURE */
#include <unordered_map>

#include "misc_loaders.h"

int insertSynonyms(std::string file, PMGD::Graph& db, std::string tag, std::string prop)
{
    std::string line;
    std::string token;

    std::ifstream filein(file);
    std::vector<std::string> tokens;
    int labelCounter = 0;

    PMGD::Transaction txi(db, PMGD::Transaction::ReadWrite);
    db.create_index(PMGD::Graph::NodeIndex, tag.c_str(), prop.c_str(), PMGD::PropertyType::String);
    txi.commit();

    while(std::getline(filein, line))
    {     // '\n' is the default delimiter
        PMGD::Transaction tx(db, PMGD::Transaction::ReadWrite);

        std::istringstream iss(line);
        tokens.clear();

        while(std::getline(iss, token, '\t'))
        {
            tokens.push_back(token);
        }

        if (tokens.size() != 2)
        {
            std::cout << "Problem loading Synonyms: " << std::endl;
            std::cout << "Tokens different from 2, something went wrong... " << std::endl;
            continue;
        }

        for (size_t i = 0; i < tokens.size(); ++i)
        {
            PMGD::PropertyPredicate pps1(prop.c_str(), PMGD::PropertyPredicate::Eq, tokens[i].c_str() );
            PMGD::NodeIterator nodeit = db.get_nodes(tag.c_str(), pps1);

            if (!nodeit)
            {
                PMGD::Node &nlabel = db.add_node(tag.c_str());
                nlabel.set_property(prop.c_str(), tokens[i].c_str());
            }
        }

        PMGD::PropertyPredicate pps1(prop.c_str(), PMGD::PropertyPredicate::Eq, tokens[0].c_str() );
        PMGD::NodeIterator i_first = db.get_nodes(tag.c_str(), pps1);

        PMGD::PropertyPredicate pps2(prop.c_str(), PMGD::PropertyPredicate::Eq, tokens[1].c_str() );
        PMGD::NodeIterator i_second = db.get_nodes(tag.c_str(), pps2);

        if (!i_first || !i_second)
        {
            std::cout << "Label not found, what?" << std::endl;
        }

        PMGD::Edge &e = db.add_edge(*i_first, *i_second, "synonym");

        tx.commit();
    }

    PMGD::Transaction tx(db, PMGD::Transaction::ReadWrite);
    for (PMGD::NodeIterator i = db.get_nodes(tag.c_str()); i; i.next())
    {
        labelCounter++;
    }
    tx.commit();
    std::cout << "Total Labels and synonyms: " << labelCounter << std::endl;

    return 0;
}

int insertCountriesTree(std::string file, PMGD::Graph& db, std::string tag)
{
    std::string line;
    std::string token;

    std::ifstream filein(file);
    std::vector<std::string> tokens;
    int placesCounter = 0;

    PMGD::Transaction txrem(db, PMGD::Transaction::ReadWrite);
    for (PMGD::NodeIterator i = db.get_nodes(tag.c_str()); i; i.next()) // remove all places
    {
        db.remove(*i);
    }
    txrem.commit();

    PMGD::Transaction txi(db, PMGD::Transaction::ReadWrite);
    db.create_index(PMGD::Graph::NodeIndex, tag.c_str(), "name",      PMGD::PropertyType::String);
    db.create_index(PMGD::Graph::NodeIndex, tag.c_str(), "type",      PMGD::PropertyType::String);
    db.create_index(PMGD::Graph::NodeIndex, tag.c_str(), "code",      PMGD::PropertyType::String);
    db.create_index(PMGD::Graph::NodeIndex, tag.c_str(), "latitude" , PMGD::PropertyType::Float);
    db.create_index(PMGD::Graph::NodeIndex, tag.c_str(), "longitude", PMGD::PropertyType::Float);
    txi.commit();

    std::unordered_map<std::string, std::unordered_map<std::string,int > > continents; //List of countries by continent.
    std::unordered_map<std::string, std::string> continents_code; // Continent Code map
    std::unordered_map<std::string, std::string> countries_code;  // Country Code map

    while(std::getline(filein, line))
    {     // '\n' is the default delimiter

        std::istringstream iss(line);
        tokens.clear();
        while(std::getline(iss, token, ','))
        {
            tokens.push_back(token);
        }

        continents[tokens[3]][tokens[5]] = 1;
        continents_code[tokens[3]] = tokens[2];
        countries_code [tokens[5]] = tokens[4];
    }


    for ( auto it = continents.begin(); it != continents.end(); ++it )
    {

        PMGD::Transaction tx(db, PMGD::Transaction::ReadWrite);

        PMGD::Node &continent = db.add_node(tag.c_str());
        continent.set_property("type", "continent");
        continent.set_property("name", it->first.c_str());
        continent.set_property("code", continents_code[it->first].c_str());

        for ( auto it_c = it->second.begin(); it_c != it->second.end(); ++it_c )
        {
            PMGD::Node &country = db.add_node(tag.c_str());
            country.set_property("type", "country");
            country.set_property("name", it_c->first.c_str());
            country.set_property("code", countries_code[it_c->first].c_str());

            db.add_edge(country, continent, "belongs to");
        }

        tx.commit();

    }

    PMGD::Transaction tx(db, PMGD::Transaction::ReadWrite);
    for (PMGD::NodeIterator i = db.get_nodes(tag.c_str()); i; i.next()) // should be only 0 or 1 though
    {
        placesCounter++;
        std::cout << i->get_property("type").string_value() << " "
                  << i->get_property("code").string_value() << " "
                  << i->get_property("name").string_value() << std::endl;
    }
    tx.commit();
    std::cout << "Total Places: " << placesCounter << std::endl;

    return 0;
}

int insertCities(std::string file, PMGD::Graph& db, std::string tag)
{
    std::string line;
    std::string token;

    std::ifstream filein(file);
    std::vector<std::string> tokens;
    int placesCounter = 0;

    while(std::getline(filein, line))
    {     // '\n' is the default delimiter

        PMGD::Transaction tx(db, PMGD::Transaction::ReadWrite);

        std::istringstream iss(line);
        tokens.clear();

        while(std::getline(iss, token, ','))
        {
            tokens.push_back(token);
        }

        PMGD::PropertyPredicate pps1("code", PMGD::PropertyPredicate::Eq, tokens[1].c_str() );
        PMGD::NodeIterator i = db.get_nodes(tag.c_str(), pps1);

        if (i)
        {
            if (tokens[3].empty()) // No city name, assigning lat and long to the country
            {
                i->set_property("latitude",  atof(tokens[5].c_str()));
                i->set_property("longitude", atof(tokens[6].c_str()));
            }
            else // New city in that country
            {
                PMGD::PropertyPredicate pps2("name", PMGD::PropertyPredicate::Eq, tokens[3].c_str() );
                PMGD::NodeIterator i_city = db.get_nodes(tag.c_str(), pps2);

                if (!i_city) // City does not exist already
                {
                    PMGD::Node &city = db.add_node(tag.c_str());
                    city.set_property("type", "city");
                    city.set_property("name", (tokens[3].c_str()));
                    city.set_property("code", "nocitycode");
                    city.set_property("latitude",  atof(tokens[5].c_str()));
                    city.set_property("longitude", atof(tokens[6].c_str()));

                    db.add_edge(city, *i, "belongs to");
                }

            }
        }
        else
        {
            std::cout << "Non-existent Country code: " << tokens[1] << std::endl;
        }

        tx.commit();
    }

    PMGD::Transaction tx(db, PMGD::Transaction::ReadWrite);
    for (PMGD::NodeIterator i = db.get_nodes(tag.c_str()); i; i.next()) // should be only 0 or 1 though
    {
        placesCounter++;
    }
    tx.commit();

    std::cout << "Total Places: " << placesCounter << std::endl;

    return 0;
}



