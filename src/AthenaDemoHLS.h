#pragma once
#define __STDC_FORMAT_MACROS

#include <string>
#include <unordered_map>
#include <functional>
#include <vector>
#include <list>
#include <unordered_map>
#include <map>

#include "VCL.h"
#include "jarvis.h"

class AthenaDemoHLS{

public:

    typedef std::list<std::reference_wrapper<Jarvis::Node>> NodeList;
    // < DrugName, <efective, not-efective> >
    typedef std::map<std::string,std::pair<int, int>> Histogram;

    enum FormatMode{ TDB, EXT4};
    AthenaDemoHLS(std::string graph_path, FormatMode mode,
                            std::string db_path);

    void runQuery1(std::string treatment);
    void runQuery2(int age_low, int age_high);
    void runQuery3(std::string name);
    void runQuery4(std::string patient);
    void runQuery5(std::string patient);
    void runQuery6(std::string patient);

private:

    Jarvis::Graph* _db;
    std::string _tdb_path;
    std::string _png_path;
    FormatMode _mode;

    int getKarnofskyScore(Jarvis::Node& node);
    Histogram bestTreatment(std::string tag, std::string name);

    std::string getPatientImagesName(std::string pat_id);
    std::string getPatientFollowups(std::string pat_id);
    Histogram bestDrugTreatment();
    Histogram bestRadTreatment();
    Histogram bestTreatmentByAge(int age_low, int age_high);
    float drugPerformance(std::string name);


    std::vector<VCL::Image> getImagesForPatient(std::string patient);
    std::vector<VCL::Image> getImagesForPatient(std::string patient,
                                                    int threshold);
    std::vector<VCL::Image> getImagesForPatient(std::string patient,
                                                    int threshold,
                                                    VCL::Rectangle rect);
    std::vector<VCL::Image> getImagesForPatient(std::string patient,
                                                    VCL::Rectangle rect);

};
