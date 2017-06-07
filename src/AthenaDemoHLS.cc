#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <fstream>
#include <string>

#include <stdlib.h>     /* system, NULL, EXIT_FAILURE */

#include "util.h"
#include "neighbor.h"

#include "AthenaDemoHLS.h"
#include "chrono/Chrono.h"

// NODE TAGS
#define PATIENT_TAG         "patient"
#define FOLLOWUP_TAG        "follow_up"
#define RAD_TREATMENT_TAG   "rad_treatment"
#define DRUG_TREATMENT_TAG  "drug_treatment"
#define OMF_TAG             "omf"
#define NTE_TAG             "nte"
#define IMAGE_SET_TAG       "image_set"  // New Tumor Event

// EDGE TAGS
#define FOLLOW_UP_EDGE_TAG      "had_follow_up"
#define RAD_TREATMENT_EDGE_TAG  "had_r_trearment"
#define DRUG_TREATMENT_EDGE_TAG "had_d_trearment"
#define OMF_EDGE_TAG            "had_omf"
#define NTE_EDGE_TAG            "had_nte"
#define IMAGE_SET_EDGE_TAG      "had_scan"

// OTHERS
#define PATIENT_ID_PROP     "bcr_patient_barc"
#define PATIENT_AGE_PROP    "age_at_initial"
#define KARNOFKY_SCORE_ID   "karnofsky_score"
#define DATE_FORM_ID        "form_completion_"
#define DRUG_NAME_ID        "drug_name"
#define RAD_NAME_ID         "radiation_type"

#define IMAGES_PER_SCAN 150

AthenaDemoHLS::AthenaDemoHLS(std::string graph_path, FormatMode mode,
                             std::string db_path):
    _mode(mode)
{
    if(_mode == TDB) {
        _tdb_path = db_path;
    }
    else if (_mode == EXT4){
        _png_path = db_path;
    }

    Jarvis::Graph::Config pmgd_config;
    pmgd_config.default_region_size = 0x1000000000;

    try{
        _db = new Jarvis::Graph(graph_path.c_str(),
            Jarvis::Graph::ReadWrite, &pmgd_config);
    }
    catch(Jarvis::Exception e) {
        print_exception(e);

        try{
            _db = new Jarvis::Graph(graph_path.c_str(),
                Jarvis::Graph::Create, &pmgd_config);
        }
        catch(Jarvis::Exception e) {
            print_exception(e);
            printf("FATAL ERROR OPENING/CREATING PMGD DATABASE\n");
        }
    }
}

bool compare_dates (const std::reference_wrapper<Jarvis::Node>& first,
                    const std::reference_wrapper<Jarvis::Node>& second)
{
    uint64_t date_first  = 0;
    uint64_t date_second = 0;

    Jarvis::PropertyIterator props = first.get().get_properties();
    for (Jarvis::PropertyIterator p = props; p; p.next()){
        if (p->type()==Jarvis::PropertyType::Time){
            date_first = p->time_value().get_time_in_msec();
        }
    }

    Jarvis::PropertyIterator props2 = second.get().get_properties();
    for (Jarvis::PropertyIterator p = props2; p; p.next()){
        if (p->type()==Jarvis::PropertyType::Time){
            date_second = p->time_value().get_time_in_msec();
        }
    }

    if (date_first == 0 || date_second == 0) {
        std::cout << "Date 0" << std::endl;
        exit(0);
    }

    return ( date_first < date_second );
}

std::string AthenaDemoHLS::getPatientImagesName(std::string pat_id)
{
    std::string image;

    try{
        Jarvis::Transaction tx(*_db, Jarvis::Transaction::ReadOnly);

        Jarvis::PropertyPredicate pps1(PATIENT_ID_PROP,
                        Jarvis::PropertyPredicate::Eq, pat_id.c_str());

        Jarvis::NodeIterator patient = _db->get_nodes(PATIENT_TAG, pps1);

        if (!patient){
            std::cout << "Patient " << pat_id << " Not Found" << std::endl;
            return std::string("NOT FOUND");
        }

        Jarvis::NodeIterator ne = get_neighbors(*patient,
                                        Jarvis::Any, IMAGE_SET_TAG);

        // Jarvis::PropertyIterator props = ne->get_properties();
        // std::cout << ne->get_tag().name() << std::endl;
        // for (Jarvis::PropertyIterator i = props; i; i.next()){
        //     std::cout << i->string_value() << std::endl;
        // }

        for (Jarvis::NodeIterator i = ne; i; i.next()){

            if (i->get_tag().name() == IMAGE_SET_TAG){
                image = i->get_property("png_name").string_value();
                // std::cout << pat_id << " : " << image << std::endl;
            }
        }

        tx.commit();
    }
    catch (Jarvis::Exception e) {
        print_exception(e);
    }

    return image;
}

std::string AthenaDemoHLS::getPatientFollowups(std::string pat_id)
{
    std::string image;

    try{
        Jarvis::Transaction tx(*_db, Jarvis::Transaction::ReadOnly);

        Jarvis::PropertyPredicate pps1(PATIENT_ID_PROP,
                        Jarvis::PropertyPredicate::Eq, pat_id.c_str());

        Jarvis::NodeIterator patient = _db->get_nodes(PATIENT_TAG, pps1);

        if (!patient){
            std::cout << "ERROR: Patient " << pat_id
                      << " Not Found" << std::endl;
            return std::string("NOT FOUND");
        }

        Jarvis::NodeIterator ne = get_neighbors(*patient,
                                    Jarvis::Any, IMAGE_SET_TAG);

        NodeList event_list;

        uint64_t drug_time = 0;

        for (Jarvis::NodeIterator i = ne; i; i.next()){

            if (i->get_tag().name() == DRUG_TREATMENT_TAG){
                event_list.push_back(*i);
            }
            if (i->get_tag().name() == FOLLOWUP_TAG){
                event_list.push_back(*i);
            }
        }

        event_list.sort(compare_dates);

        for (auto& node:event_list) {
            std::cout << node.get().get_tag().name() << ": " ;
            Jarvis::PropertyIterator props2 = node.get().get_properties();
            uint64_t date;
            for (Jarvis::PropertyIterator p = props2; p; p.next()){
                if (p->type()==Jarvis::PropertyType::Time){
                    date = p->time_value().get_time_in_msec();
                    std::cout << date << std::endl;
                }
            }
        }

        tx.commit();
    }
    catch (Jarvis::Exception e) {
        print_exception(e);
    }

    return image;
}

int AthenaDemoHLS::getKarnofskyScore(Jarvis::Node& node)
{
    int default_score = 100;

    std::string str_score;
    Jarvis::Property prop;
    if (node.check_property(KARNOFKY_SCORE_ID, prop))
        str_score = prop.string_value();
    else
        return default_score;

    if (str_score.compare("[Not Available]") == 0 ||
        str_score.compare("[Unknown]") == 0 ||
        str_score.compare("[Not Evaluated]") == 0)
        return default_score;

    return std::stoi(str_score);
}

/*
Query: Most effective drug/radiation_type after first treatment

Check FOLLOWUP_VISIT that happened after a DRUG_TREATMENT/RADIATION_TREATMENT
and look for improvements in tumor_status and karnofsky_score
*/
AthenaDemoHLS::Histogram AthenaDemoHLS::bestTreatment(std::string t_tag,
                                              std::string t_name)
{
    std::string drug;
    Histogram histo;

    try{
        Jarvis::Transaction tx(*_db, Jarvis::Transaction::ReadOnly);

        Jarvis::NodeIterator patients = _db->get_nodes(PATIENT_TAG);

        if (!patients){
            std::cout << "ERROR: Patients " << std::endl;
            return histo;
        }

        for (Jarvis::NodeIterator pat = patients; pat; pat.next()){

            Jarvis::NodeIterator ne = get_neighbors(*pat, Jarvis::Any);

            int pat_score = getKarnofskyScore(*pat);

            NodeList event_list;
            for (Jarvis::NodeIterator i = ne; i; i.next()){

                if (i->get_tag().name() == t_tag){
                    event_list.push_back(*i);
                }
                if (i->get_tag().name() == FOLLOWUP_TAG){
                    event_list.push_back(*i);
                }
            }

            // Sort treatment and followup by date.
            event_list.sort(compare_dates);
            for (auto it = event_list.begin(); it!= event_list.end(); ++it) {

                if ((*it).get().get_tag().name().compare(t_tag) == 0) {

                    auto it_aux = it;
                    it_aux++;

                    std::string name = (*it).get().
                                get_property(t_name.c_str()).string_value();

                    if ( histo.find(name) == histo.end() ) {
                        std::pair<int,int> p_aux(0,0);
                        histo[name] = p_aux;
                    }
                    std::pair<int,int>& p = histo[name];

                    while(it_aux != event_list.end()){
                        if ( (*it_aux).get().get_tag().name().
                                    compare(FOLLOWUP_TAG) == 0 &&
                            compare_dates((*it), (*it_aux))) {

                            if(getKarnofskyScore(*it_aux) > pat_score){
                                p.first++;
                            }
                            else{
                                p.second++;
                            }
                            break;
                        }
                        it_aux++;
                    }
                }
            }
        }

        tx.commit();
    }
    catch (Jarvis::Exception e) {
        print_exception(e);
    }

    return histo;
}

float AthenaDemoHLS::drugPerformance(std::string name)
{
    std::string drug;
    Histogram histo;

    // try{
    //     Jarvis::Transaction tx(*_db, Jarvis::Transaction::ReadOnly);

    //     Jarvis::NodeIterator patients = _db->get_nodes(PATIENT_TAG);

    //     if (!patients){
    //         std::cout << "ERROR: Patients " << std::endl;
    //         return 0;
    //     }

    //     for (Jarvis::NodeIterator pat = patients; pat; pat.next()){

    //         Jarvis::NodeIterator ne = get_neighbors(*pat, Jarvis::Any);

    //         int pat_score = getKarnofskyScore(*pat);

    //         NodeList event_list;
    //         for (Jarvis::NodeIterator i = ne; i; i.next()){

    //             if (i->get_tag().name() == DRUG_TREATMENT_TAG){
    //                 if (i->get_property(DRUG_NAME_ID.c_str()).
    //                                         string_value() == name)
    //                     event_list.push_back(*i);
    //             }
    //             if (i->get_tag().name() == FOLLOWUP_TAG){
    //                 event_list.push_back(*i);
    //             }
    //         }

    //         // Sort treatment and followup by date.
    //         event_list.sort(compare_dates);
    //         for (auto it = event_list.begin(); it!= event_list.end(); ++it) {

    //             if ((*it).get().get_tag().name().
    //                                 compare(DRUG_TREATMENT_TAG) == 0) {

    //                 auto it_aux = it;
    //                 it_aux++;

    //                 std::string name = (*it).get().
    //                             get_property(DRUG_NAME_ID.c_str()).string_value();

    //                 if ( histo.find(name) == histo.end() ) {
    //                     std::pair<int,int> p_aux(0,0);
    //                     histo[name] = p_aux;
    //                 }
    //                 std::pair<int,int>& p = histo[name];

    //                 while(it_aux != event_list.end()){
    //                     if ( (*it_aux).get().get_tag().name().
    //                                 compare(FOLLOWUP_TAG) == 0 &&
    //                         compare_dates((*it), (*it_aux))) {

    //                         if(getKarnofskyScore(*it_aux) > pat_score){
    //                             p.first++;
    //                         }
    //                         else{
    //                             p.second++;
    //                         }
    //                         break;
    //                     }
    //                     it_aux++;
    //                 }
    //             }
    //         }
    //     }

    //     tx.commit();
    // }
    // catch (Jarvis::Exception e) {
    //     print_exception(e);
    // }

    return 1;
}

AthenaDemoHLS::Histogram AthenaDemoHLS::bestRadTreatment()
{
    return bestTreatment(RAD_TREATMENT_TAG, RAD_NAME_ID);
}

AthenaDemoHLS::Histogram AthenaDemoHLS::bestDrugTreatment()
{
    return bestTreatment(DRUG_TREATMENT_TAG, DRUG_NAME_ID);
}

AthenaDemoHLS::Histogram AthenaDemoHLS::bestTreatmentByAge(int age_low,
                                                           int age_high)
{
    std::string drug;
    Histogram histo;

    if (age_low > age_high) {
        std::cout << "Age range makes no sense" << std::endl;
        return histo;
    }

    try{
        Jarvis::Transaction tx(*_db, Jarvis::Transaction::ReadWrite);

        Jarvis::PropertyPredicate pps1(PATIENT_AGE_PROP,
                Jarvis::PropertyPredicate::GeLe, age_low, age_high);
        Jarvis::NodeIterator patients = _db->get_nodes(PATIENT_TAG, pps1);

        if (!patients){
            std::cout << "ERROR: Not enough Patients " << std::endl;
            return histo;
        }

        for (Jarvis::NodeIterator pat = patients; pat; pat.next()){

            Jarvis::NodeIterator ne = get_neighbors(*pat, Jarvis::Any);

            NodeList event_list;
            std::string name;
            for (Jarvis::NodeIterator i = ne; i; i.next()){

                if (i->get_tag().name() == DRUG_TREATMENT_TAG){
                    name = "DRUG_";
                    name += i->get_property(DRUG_NAME_ID).string_value();
                }
                if (i->get_tag().name() == RAD_TREATMENT_TAG){
                    name = "RAD_";
                    name += i->get_property(RAD_NAME_ID).string_value();
                }
            }

            if (name.empty()) {
                continue;
            }

            if ( histo.find(name) == histo.end() ) {
                std::pair<int,int> p_aux(0,0);
                histo[name] = p_aux;
            }
            std::pair<int,int>& p = histo[name];
            p.first++;
            p.second++;
        }

        tx.commit();
    }
    catch (Jarvis::Exception e) {
        print_exception(e);
    }

    return histo;
}

std::vector<VCL::Image> AthenaDemoHLS::getImagesForPatient(std::string patient)
{
    std::vector<VCL::Image> res;

    std::string image_name = getPatientImagesName(patient);

    if (image_name.empty()) {
        std::cout << "Patient not found: "<< patient << std::endl;
        return res;
    }

    try{
        for (int i = 0; i < IMAGES_PER_SCAN; ++i)
        {
            std::stringstream ss;
            ss << std::setw(4) << std::setfill('0') << i;
            std::string s = ss.str();
            std::string filename;

            if (_mode == TDB) {
                filename = _tdb_path + image_name + ".nii." + s + ".tdb";
            }
            else if (_mode == EXT4) {
                filename = _png_path + image_name + ".nii." + s + "..png";
            }

            res.push_back(VCL::Image(filename));
        }
    }
    catch (VCL::Exception e) {
        //print_exception(e);
        std::cout << "Eccep" << std::endl;
    }

    return res;
}

std::vector<VCL::Image> AthenaDemoHLS::getImagesForPatient(std::string patient,
                            VCL::Rectangle r)
{
    std::vector<VCL::Image> res;

    std::string image_name = getPatientImagesName(patient);
    // std::cout << image_name << std::endl;

    if (image_name.empty()) {
        std::cout << "Patient not found: "<< patient << std::endl;
        return res;
    }

    try{
        for (int i = 0; i < IMAGES_PER_SCAN; ++i) {
            std::stringstream ss;
            ss << std::setw(4) << std::setfill('0') << i;
            std::string s = ss.str();
            std::string filename;

            if (_mode == TDB) {
                filename = _tdb_path + image_name + ".nii." + s + ".tdb";
                // std::cout << filename << std::endl;
            }
            else if (_mode == EXT4) {
                filename = _png_path + image_name + ".nii." + s + "..png";
            }

            VCL::Image img(filename);
            img.crop(r);
            // cv::Mat aux = img.get_cvmat();
            res.push_back(img);
        }
    }
    catch (VCL::Exception e) {
        printf("[Exception] %s at %s:%d\n", e.name, e.file, e.line);
            if (e.errno_val != 0)
                printf("%s: %s\n", e.msg.c_str(), strerror(e.errno_val));
            else if (!e.msg.empty())
                printf("%s\n", e.msg.c_str());
    }

    return res;
}

std::vector<VCL::Image> AthenaDemoHLS::getImagesForPatient(std::string patient,
                            int threshold)
{
    std::vector<VCL::Image> res;

    std::string image_name = getPatientImagesName(patient);
    // std::cout << image_name << std::endl;

    if (image_name.empty()) {
        std::cout << "Patient not found: "<< patient << std::endl;
        return res;
    }

    try{
        for (int i = 0; i < IMAGES_PER_SCAN; ++i) {
            std::stringstream ss;
            ss << std::setw(4) << std::setfill('0') << i;
            std::string s = ss.str();
            std::string filename;

            if (_mode == TDB) {
                filename = _tdb_path + image_name + ".nii." + s + ".tdb";
            }
            else if (_mode == EXT4) {
                filename = _png_path + image_name + ".nii." + s + "..png";
            }

            VCL::Image img(filename);
            img.threshold(threshold);
            res.push_back(img);
        }
    }
    catch (VCL::Exception e) {
        printf("[Exception] %s at %s:%d\n", e.name, e.file, e.line);
            if (e.errno_val != 0)
                printf("%s: %s\n", e.msg.c_str(), strerror(e.errno_val));
            else if (!e.msg.empty())
                printf("%s\n", e.msg.c_str());
    }

    return res;
}

std::vector<VCL::Image> AthenaDemoHLS::getImagesForPatient(std::string patient,
                            int threshold,
                            VCL::Rectangle r)
{
    std::vector<VCL::Image> res;

    std::string image_name = getPatientImagesName(patient);
    // std::cout << image_name << std::endl;

    if (image_name.empty()) {
        std::cout << "Patient not found: "<< patient << std::endl;
        return res;
    }

    try{
        for (int i = 0; i < IMAGES_PER_SCAN; ++i) {
            std::stringstream ss;
            ss << std::setw(4) << std::setfill('0') << i;
            std::string s = ss.str();
            std::string filename;

            if (_mode == TDB) {
                filename = _tdb_path + image_name + ".nii." + s + ".tdb";
            }
            else if (_mode == EXT4) {
                filename = _png_path + image_name + ".nii." + s + "..png";
            }

            VCL::Image img(filename);
            img.crop(r);
            img.threshold(threshold);
            res.push_back(img);
        }
    }
    catch (VCL::Exception e) {
        printf("[Exception] %s at %s:%d\n", e.name, e.file, e.line);
            if (e.errno_val != 0)
                printf("%s: %s\n", e.msg.c_str(), strerror(e.errno_val));
            else if (!e.msg.empty())
                printf("%s\n", e.msg.c_str());
    }

    return res;
}

void AthenaDemoHLS::runQuery1(std::string treatment)
{
    AthenaDemoHLS::Histogram histo;
    ChronoCpu chrono("Follow_query");
    for (int i = 0; i < 50; ++i) {

        if (treatment == "drug") {
            chrono.tic();
            histo = bestDrugTreatment();
            chrono.tac();
        }
        else{
            chrono.tic();
            histo = bestRadTreatment();
            chrono.tac();
        }

    }

    std::cout << "Query Time[us]: " << chrono.getAvgTime_us()
              << " - std[us]: " << chrono.getSTD_us() << std::endl;

    for (auto& drug:histo) {
        if (drug.second.first + drug.second.second > 1) {
            std::cout << drug.first << ": "
                         << (float)(drug.second.first * 100)
                            / (float)(drug.second.first + drug.second.second)
                         << " | Samples: "
                         << (drug.second.first + drug.second.second)
                   << std::endl;
        }
    }
}

void AthenaDemoHLS::runQuery2(int age_low, int age_high)
{
    AthenaDemoHLS::Histogram histo;
    ChronoCpu chrono_age("chrono");
    for (int i = 0; i < 50; ++i) {
        chrono_age.tic();
        histo = bestTreatmentByAge(age_low,age_high);
        chrono_age.tac();
    }

    std::cout << "Query Time[us]: " << chrono_age.getAvgTime_us()
              << " - std[us]: " << chrono_age.getSTD_us() << std::endl;

    for (auto& treat:histo) {
        std::cout << treat.first << ": " << treat.second.first << std::endl;
    }
}

void AthenaDemoHLS::runQuery3(std::string name)
{
    std::cout << std::endl << "Running Query 3..." << std::endl;

    AthenaDemoHLS::Histogram histo;
    ChronoCpu chrono("Follow_query");
    for (int i = 0; i < 50; ++i) {
        chrono.tic();
        histo = bestDrugTreatment();
        chrono.tac();
    }

    std::cout << "Query Time[us]: " << chrono.getAvgTime_us()
              << " - std[us]: " << chrono.getSTD_us() << std::endl;

    bool flag = false;
    for (auto& drug:histo) {
        if (name == drug.first)
        {
            flag = true;
            std::cout<< "Effectiveness of " << name << ": "
                     << (float)(drug.second.first * 100)
                        / (float)(drug.second.first + drug.second.second)
                     << " | Samples: "
                     << (drug.second.first + drug.second.second)
                     << std::endl;
        }
    }

    if (!flag)
        std::cout << "Drug Not found" << std::endl;
}

void AthenaDemoHLS::runQuery4(std::string patient)
{
    VCL::Rectangle r(60,60,60,60);

    std::vector<VCL::Image> res;
    ChronoCpu chrono("chrono");
    for (int i = 0; i < 10; ++i)
    {
        chrono.tic();
        res = getImagesForPatient(patient, r);
        // std::vector<VCL::Image> res = hlsdb.getImagesForPatient(patient);
        chrono.tac();
    }
    std::cout << "Query Time[ms]: " << chrono.getAvgTime_ms()
              << " - std[ms]: " << chrono.getSTD_ms() << std::endl;

    std::cout << "Saving images in ./resutls/crop_XXX" << std::endl;

    for (int i = 0; i < res.size(); ++i)
    {
        try{
            res.at(i).store(VCL::PNG,
                            "results/crop_"+ patient + std::to_string(i));
        }
        catch (VCL::Exception e) {
            printf("[Exception] %s at %s:%d\n", e.name, e.file, e.line);
            if (e.errno_val != 0)
                printf("%s: %s\n", e.msg.c_str(), strerror(e.errno_val));
            else if (!e.msg.empty())
                printf("%s\n", e.msg.c_str());
        }
    }
}

void AthenaDemoHLS::runQuery5(std::string patient)
{
    float threshold = 150;

    std::vector<VCL::Image> res;
    ChronoCpu chrono("chrono");
    for (int i = 0; i < 10; ++i)
    {
        chrono.tic();
        res = getImagesForPatient(patient, threshold);
        // std::vector<VCL::Image> res = hlsdb.getImagesForPatient(patient);
        chrono.tac();
    }
    std::cout << "Query Time[ms]: " << chrono.getAvgTime_ms()
              << " - std[ms]: " << chrono.getSTD_ms() << std::endl;

    std::cout << "Saving images in ./resutls/thre_XXX" << std::endl;
    for (int i = 0; i < res.size(); ++i)
    {
        try{
            res.at(i).store(VCL::PNG,
                            "results/thre_"+ patient + std::to_string(i));
        }
        catch (VCL::Exception e) {
            printf("[Exception] %s at %s:%d\n", e.name, e.file, e.line);
            if (e.errno_val != 0)
                printf("%s: %s\n", e.msg.c_str(), strerror(e.errno_val));
            else if (!e.msg.empty())
                printf("%s\n", e.msg.c_str());
        }
    }
}

void AthenaDemoHLS::runQuery6(std::string patient)
{
    float threshold = 150;
    VCL::Rectangle r(100, 100, 10, 10);
    std::vector<VCL::Image> res;

    ChronoCpu chrono("chrono");
    for (int i = 0; i < 10; ++i)
    {
        chrono.tic();
        res = getImagesForPatient(patient, threshold, r);
        chrono.tac();
    }
    std::cout << "Query Time[ms]: " << chrono.getAvgTime_ms()
              << " - std[ms]: " << chrono.getSTD_ms() << std::endl;

    std::cout << "Saving images in ./resutls/crop_thre_XXX" << std::endl;
    for (int i = 0; i < res.size(); ++i)
    {
        try{
            res.at(i).store(VCL::PNG,
                            "results/crop_thre_"+ patient + std::to_string(i));
        }
        catch (VCL::Exception e) {
            printf("[Exception] %s at %s:%d\n", e.name, e.file, e.line);
            if (e.errno_val != 0)
                printf("%s: %s\n", e.msg.c_str(), strerror(e.errno_val));
            else if (!e.msg.empty())
                printf("%s\n", e.msg.c_str());
        }
    }
}
