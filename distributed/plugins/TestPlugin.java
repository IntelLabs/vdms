import java.io.IOException;

import java.lang.System;

import java.net.Socket;
import java.net.UnknownHostException;

import java.nio.ByteBuffer;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;

import org.json.simple.JSONObject;
import org.json.simple.JSONValue;
import org.json.simple.JSONArray;
import org.json.simple.parser.JSONParser;
import org.json.simple.parser.ParseException;

public class TestPlugin extends Plugin
{
    
    public TestPlugin(String configFileName)
    {
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
    
    public static void main (String[] args)
    {
        new TestPlugin("debug/replication_config.json");
    }
    
}

