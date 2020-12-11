import java.io.IOException;

import java.lang.System;

import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;

import java.util.ArrayList;

import org.json.simple.JSONObject;
import org.json.simple.parser.JSONParser;
import org.json.simple.parser.ParseException;

public class TestFilter extends Plugin
{
    public TestFilter(String configFileName)
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

            String filterConfigFileName = (String) config.get("filter_config_file");
            ArrayList<PassList> allFilterFields  = LoadFilterFields(filterConfigFileName);

            //match the listId of the passLists specified in the configuration file with the appropriate thread
            for(int i = 0; i < allFilterFields.size(); i++)
            {
                for(int j = 0; j < subscriberList.size(); j++)
                {
                    if(allFilterFields.get(i).GetListId() == subscriberList.get(j).GetPassList().GetListId())
                    {
                        subscriberList.get(j).SetPassList(allFilterFields.get(i));
                    }

                }
            }
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
    
    public ArrayList<PassList> LoadFilterFields(String filterConfigFileName)
    {
        
        return null;
    }
    
    public static void main (String[] args)
    {
        new TestFilter("debug/filter_config.json");
    }
    
}

