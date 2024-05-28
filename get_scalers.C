#include "TSQLServer.h"
#include "TSQLResult.h"
#include "TSQLRow.h"
#include "TTree.h"
#include <string>
#include <iostream>
using namespace std;

// runnum, sd,r,l,w, events, duration, starttime
void get_scalers(int firstrun = 40000, vector<string> runtypes = {}, vector<int> vGTMs = {}) {
    TSQLServer *db = TSQLServer::Connect("pgsql://10.20.32.23:5432/daq","",""); // IP of database server
    TSQLResult *res;
    TSQLRow * row;
    
    if (db) printf("Server info: %s\n", db->ServerInfo());
    else return "no data";

    string query = string("SELECT R.eventsinrun, R.brtimestamp, R.ertimestamp, SD.* FROM run R ")
                 + "INNER JOIN gl1_scaledown SD ON R.runnumber = SD.runnumber ";
    /*
    if (vGTMs.size() != 0) {
        query = query + "INNER JOIN gtm G ON R.runnumber = G.runnumber AND (";
        for (int i = 0; i < vGTMs.size(); i++) { 
            if (i != 0) query = query + "AND ";
            query = query + "G.vgtm = " + to_string(vGTMs.at(i)) + " ";
        }
        query = query + ") AND G.global = \'t\' ";
    }
    */
    if (runtypes.size() != 0) {
        query = query + "AND (";
        for (int i = 0; i < runtypes.size(); i++) {
            if (i != 0) query = query + "OR ";
            query = query + "R.runtype = \'" + runtypes.at(i) + "\' ";
        }
        query += ") ";
    }
    query += "AND R.runnumber >= "+ to_string(firstrun) + " ORDER BY SD.runnumber";
    cout << query << endl;
    const char * sql = query.c_str();
    res = db->Query(sql);
    int nrows = res->GetRowCount();
    int nfields = res->GetFieldCount();
   
    TTree * t = new TTree("scalers","scalers for all runs");
    
    int t_scaledowns[64] = { 0 };
    long long t_raw[64] = { 0 };
    long long t_live[64] = { 0 };
    long long t_scaled[64] = { 0 };
    int t_runnumber = 0;
    int t_events = 0;
    int t_duration = 0;
    int t_starttime = 0;
    int t_endtime = 0;

    t->Branch("runnumber",&t_runnumber);
    t->Branch("events",&t_events);
    t->Branch("duration",&t_duration);
    t->Branch("starttime",&t_starttime);
    t->Branch("endtime",&t_endtime);
    t->Branch("scaledowns",&t_scaledowns,"scaledowns[64]/I");
    t->Branch("raw",&t_raw,"raw[64]/L");
    t->Branch("live",&t_live,"live[64]/L");
    t->Branch("scaled",&t_scaled,"scaled[64]/L");

    ofstream outfile("txts/scalers.txt");
    for (int irow = 0; irow < nrows; irow++) {
        
        row = res->Next();
        
        t_runnumber = atoi(row->GetField(3));
        string currentrun = to_string(t_runnumber);
        bool missing = false;
        for (int ivgtm = 0; ivgtm < vGTMs.size(); ivgtm++) {
            string vgtmquery = "SELECT global FROM gtm WHERE runnumber = " + currentrun + " AND vgtm = " + to_string(vGTMs.at(ivgtm));
            TSQLResult * vgtmres = db->Query(vgtmquery.c_str());
            TSQLRow * vgtmrow;
            vgtmrow = vgtmres->Next();
            if (strcmp(vgtmrow->GetField(0),"t")) {
                missing = true;
                cout << vGTMs.at(ivgtm) << " " << vgtmrow->GetField(0) << endl;
            }
            delete vgtmres;
            delete vgtmrow;
        }
        if (missing) continue;
        
        t_events = atoi(row->GetField(0));
       
        const char * starttime = (row->GetField(1));
        const char * endtime = (row->GetField(2));
        if (strlen(starttime) != 19 || strlen(endtime) != 19) {
            t_starttime = 0;
            t_endtime = 0;
            t_duration = 0;
        }
        else {
            TDatime das = TDatime(starttime);
            TDatime dae = TDatime(endtime);
            t_starttime = das.Convert();
            t_endtime = dae.Convert();
            t_duration = t_endtime-t_starttime;
        }
        

        for (int ifield = 4; ifield < nfields; ifield++) {
            t_scaledowns[ifield - 4] = atoi(row->GetField(ifield));
        }
        //for (int ifield = 0; ifield < nfields; ifield++) {
        //    cout << (row->GetField(ifield)) << " ";
        //}
        //cout << endl;
        string scalerquery = "SELECT index, raw, live, scaled from gl1_scalers where runnumber = " + currentrun;
        TSQLResult * scalerres = db->Query(scalerquery.c_str());
        TSQLRow * scalerrow;
        int scalerrows = scalerres->GetRowCount();
        int scalerfields = scalerres->GetFieldCount();
        for (int jrow = 0; jrow < scalerrows; jrow++) {
            scalerrow = scalerres->Next();
            t_raw[atoi(scalerrow->GetField(0))] = atol(scalerrow->GetField(1));      
            t_live[atoi(scalerrow->GetField(0))] = atol(scalerrow->GetField(2));      
            t_scaled[atoi(scalerrow->GetField(0))] = atol(scalerrow->GetField(3));      
            delete scalerrow;
        }
        outfile << t_runnumber << " " << t_events << " " << t_starttime << " " << t_duration << " ";
        for (int i = 0; i < 64; i++) {
            outfile << t_scaledowns[i] << " " << t_raw[i] << " " << t_live[i] << " " << t_scaled[i] << " ";
        }
        
        outfile << endl;
        t->Fill();
        
        memset(t_scaledowns, 0, 64*sizeof(t_scaledowns[0]));
        memset(t_raw, 0, 64*sizeof(t_raw[0]));
        memset(t_live, 0, 64*sizeof(t_live[0]));
        memset(t_scaled, 0, 64*sizeof(t_scaled[0]));
        t_runnumber = 0;
        t_events = 0;
        t_duration = 0;
        t_starttime = 0;
        t_endtime = 0;
           
        delete scalerres;
        delete row;
    }

    string treename = "scalers";
    for (int i = 0; i < runtypes.size(); i++) {
        treename = treename + "_" + runtypes.at(i);
    }
    for (int i = 0; i < vGTMs.size(); i++) {
        treename = treename + "_" + vGTMs.at(i);
    }
    treename = treename + ".root";
    TFile * f = TFile::Open(Form("trees/%s",treename.c_str()),"RECREATE");
    t->Write();
    
    delete f;
    delete res;
    delete db;
    
    return;
}
