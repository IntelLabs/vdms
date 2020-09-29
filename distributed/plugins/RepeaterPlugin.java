import java.io.BufferedReader;
import java.io.IOException;
import java.io.DataInputStream;
import java.io.DataOutputStream;

import java.lang.System;

import java.net.Socket;
import java.net.UnknownHostException;
import java.text.SimpleDateFormat;
import java.util.Calendar;
import java.util.List;
import java.util.Arrays;
import java.util.ArrayList;


public class RepeaterPlugin {
    
   public static void main (String[] args) {

       String sourceNodes = System.getenv("SOURCES");
       String sourceNodesArray[] = sourceNodes.split(",");
       int sourceNodeCount = sourceNodesArray.length;
       String destinationNodes = System.getenv("DESTINATIONS");
       String destinationNodesArray[] = destinationNodes.split(",");
       int destinationNodeCount = destinationNodesArray.length;
       WorkerConnectionList sourceConnections = new WorkerConnectionList();
       WorkerConnectionList destinationConnections = new WorkerConnectionList();

       List<Thread> threadList = new ArrayList();
       for(int i = 0; i < sourceNodeCount; i++)
	   {
	       String connectionInfo[] = sourceNodesArray[i].split(":");
	       int connectionPort = Integer.parseInt(connectionInfo[1]);
	       System.out.println(connectionInfo[0] + " " + Integer.toString(connectionPort));
	       try (Socket thisSocket = new Socket(connectionInfo[0], connectionPort))
		   {
		       SourceConnection thisConnection = new SourceConnection(thisSocket);
		       sourceConnections.AddWorker(thisConnection);
		       Thread sourceThread = new Thread(thisConnection);
		       threadList.add(sourceThread);
		       System.out.println("Socket is " + Boolean.toString(thisSocket.isConnected()));
		   }
	       catch (UnknownHostException ex)
		   {
		       System.out.println("Server not found: " + ex.getMessage());
		   }
	       catch (IOException ex)
		   {
		       System.out.println("I/O error: " + ex.getMessage());
		   }
	   }
       
       for(int i = 0; i < destinationNodeCount; i++)
	   {
	       String connectionInfo[] = destinationNodesArray[i].split(":");
	       int connectionPort = Integer.parseInt(connectionInfo[1]);
	       System.out.println(connectionInfo[0] + " " + Integer.toString(connectionPort));
	       try (Socket thisSocket = new Socket(connectionInfo[0], connectionPort))
		   {
		       WorkerConnection thisConnection = new WorkerConnection(thisSocket);
		       destinationConnections.AddWorker(thisConnection);
		       System.out.println("Socket is " + Boolean.toString(thisSocket.isConnected()));
		   }
	       catch (UnknownHostException ex)
		   {
		       System.out.println("Server not found: " + ex.getMessage());
		   }
	       catch (IOException ex)
		   {
		       System.out.println("I/O error: " + ex.getMessage());
		   }
	   }

       //Set the destinations for all of the source nodes
       //Start all of the threads that communicate with the Source Nodes
       for(int i = 0; i < threadList.size(); i++)
	   {
	       sourceConnections.Get(i).SetDestinations(destinationConnections);
	       threadList.get(i).start();
	   }
   }




  

}

