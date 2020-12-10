import java.io.IOException;

import java.lang.System;

import java.net.Socket;
import java.net.UnknownHostException;

import java.nio.ByteBuffer;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;

import java.util.ArrayList;

import org.json.simple.JSONObject;
import org.json.simple.JSONValue;
import org.json.simple.JSONArray;
import org.json.simple.parser.JSONParser;
import org.json.simple.parser.ParseException;

public class TestFilter extends Plugin
{
    public TestFilter(String configFileName)
    {
        ArrayList<ArrayList<String>> destinationList = new  ArrayList<ArrayList<String>>();
        ArrayList<String> destination1 = new ArrayList<String>();
        ArrayList<String> destination2 = new ArrayList<String>();
        destination1.add("AddEntity");
        destination1.add("AddConnection");
        destination1.add("FindEntity");
        destination1.add("FindConnection");
        destination1.add("UpdateEntity");
        destination1.add("UpdateConnection");
        
        destination2.add("AddImage");
        destination2.add("AddVideo");
        destination2.add("AddDescriptorSet");
        destination2.add("AddDescriptor");
        destination2.add("AddBoundingBox");
        destination2.add("FindImage");
        destination2.add("FindVideo");
        destination2.add("FindFrames");
        destination2.add("FindDescriptor");
        destination2.add("ClassifyDescriptor");
        destination2.add("FindBoundingBox");
        destination2.add("UpdateImage");
        destination2.add("UpdateVideo");
        destination2.add("UpdateBoundingBox");
        destinationList.add(destination1);
        destinationList.add(destination2);
        
        try
        {
            Path configFilePath = Paths.get(configFileName);
            String configData = Files.readString(configFilePath);
            JSONParser configParser = new JSONParser();
            
            JSONObject config = (JSONObject) configParser.parse(configData);
            String sourceConfigFileName = (String) config.get("source_config_file");
            AddPublishersFromFile(sourceConfigFileName);
            
            String destinationConfigFileName = (String) config.get("destination_config_file");
            AddSubscribersFromFile(destinationConfigFileName);
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
    
    public ArrayList<ArrayList<String>> LoadFilterFields(String filterConfigFileName)
    {
        
        return null;
        
    }
    
    public static void main (String[] args)
    {
        new TestFilter("debug/replication_config.json");
    }
    
}

