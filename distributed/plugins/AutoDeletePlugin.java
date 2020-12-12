import java.io.IOException;

import java.lang.System;

import java.net.Socket;
import java.net.UnknownHostException;

import java.nio.ByteBuffer;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;

import java.util.ArrayList;
import java.util.TimerTask;

import org.json.simple.JSONArray;
import org.json.simple.JSONObject;
import org.json.simple.parser.JSONParser;
import org.json.simple.parser.ParseException;

public class AutoDeletePlugin extends Plugin
{
    private ArrayList<PassList> allFilterFields; /**<  ArrayList holding all of the PassLists that have been created */
    
    public AutoDeletePlugin(String configFileName)
    {
        try
        {
            allFilterFields = new ArrayList<PassList>();
            Path configFilePath = Paths.get(configFileName);
            String configData = Files.readString(configFilePath);
            JSONParser configParser = new JSONParser();
            
            JSONObject config = (JSONObject) configParser.parse(configData);
            String sourceConfigFileName = (String) config.get("source_config_file");
            AddAutoPublishersFromFile(sourceConfigFileName);
            
            String destinationConfigFileName = (String) config.get("destination_config_file");
            AddSubscribersFromFile(destinationConfigFileName);
            
            String queryConfigFileName = (String) config.get("auto_queries_config_file"); //load the query parameters [FindEntity[and data]
            LoadAutoQueries(queryConfigFileName);      
        
            String filterConfigFileName = (String) config.get("filter_config_file"); //load the matching query ids
            LoadFilterFields(filterConfigFileName);

        }  

        catch(ParseException e)
        {
            e.printStackTrace();
            System.exit(-1);
        }
        catch(IOException e)
        {
            e.printStackTrace();
            System.exit(-1);
        }
        
        InitThreads();
               
    }
    
    private void LoadAutoQueries(String queryConfigFileName)
    {
        try
        {
            allFilterFields = new ArrayList<PassList>();
            Path configFilePath = Paths.get(queryConfigFileName);
            String configData = Files.readString(configFilePath);
            JSONParser configParser = new JSONParser();
            JSONArray config = (JSONArray) configParser.parse(configData);
            for(int i = 0; i < config.size(); i++)
            {
                PassList tmpPassList = new PassList(config.get(i).toString());
                allFilterFields.add(tmpPassList);
            }
            
        }
        catch(IOException e)
        {
            e.printStackTrace();
            System.exit(-1);
        }
        catch(ParseException e)
        {
            e.printStackTrace();
            System.exit(-1);
        }
    }
    
    public void AddAutoPublishersFromFile(String fileName)
    {
        try{
            JSONParser configParser = new JSONParser();
            Path configFilePath = Paths.get(fileName);
            String configData = Files.readString(configFilePath);
            JSONArray config = (JSONArray) configParser.parse(configData);
            for(int i = 0 ; i < config.size(); i++)
            {
                JSONObject connection = (JSONObject) config.get(i);
                TcpVdmsConnection tmpConnection = new TcpVdmsConnection(connection.toString());
                AutoPublisherServiceThread thisServerServivceThread = new AutoPublisherServiceThread(this, tmpConnection, threadId);
                threadId++;
                AddNewPublisher(thisServerServivceThread);
            }
        }
        catch(IOException e)
        {
            e.printStackTrace();
            System.exit(-1);
        }
        catch(ParseException e)
        {
            e.printStackTrace();
            System.exit(-1);
        }
    }
    public static void main (String[] args)
    {
        new AutoDeletePlugin("auto_query_plugin_config.json");
    }
    
    
}

