import java.io.IOException;

import java.lang.System;

import java.net.Socket;
import java.net.UnknownHostException;

import java.nio.ByteBuffer;

import java.util.ArrayList;


public class TestFilter extends Plugin
{
   public TestFilter()
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
      
    
      byte[] initSize = new byte[] {(byte)0xfc, (byte)0xff, (byte)0xff, (byte)0xff};
      byte[] initPayload = ByteBuffer.allocate(4).putInt(0).array();
      
      VdmsTransaction initSequence = new VdmsTransaction(initSize, initPayload);
      
      String sourceNodes = System.getenv("SOURCES");
      String sourceNodesArray[] = sourceNodes.split(",");
      int sourceNodeCount = sourceNodesArray.length;
      String destinationNodes = System.getenv("DESTINATIONS");
      String destinationNodesArray[] = destinationNodes.split(",");
      int destinationNodeCount = destinationNodesArray.length;
      
      for(int i = 0; i < sourceNodeCount; i++)
      {
         String connectionInfo[] = sourceNodesArray[i].split(":");
         int connectionPort = Integer.parseInt(connectionInfo[1]);
         System.out.println(connectionInfo[0] + " " + Integer.toString(connectionPort));
         try 
         {
            Socket thisSocket = new Socket(connectionInfo[0], connectionPort);
            PublisherServiceThread thisServerServivceThread = new PublisherServiceThread(this, thisSocket, threadId, initSequence);
            AddNewProducer(thisServerServivceThread);
         }
         catch (UnknownHostException e)
         {
            e.printStackTrace();
         }
         catch (IOException e)
         {
            e.printStackTrace();
         }
      }
      
      for(int i = 0; i < destinationNodeCount; i++)
      {
         String connectionInfo[] = destinationNodesArray[i].split(":");
         int connectionPort = Integer.parseInt(connectionInfo[1]);
         System.out.println(connectionInfo[0] + " " + Integer.toString(connectionPort));
         try 
         {
            Socket thisSocket = new Socket(connectionInfo[0], connectionPort);
            SubscriberServiceThread thisServerServivceThread = new SubscriberServiceThread(this, thisSocket, threadId, destinationList.get(i), null);
            AddNewConsumer(thisServerServivceThread);
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
      
      //Start all of the threads. We start the destination nodes first so that no data from the sources is acceptd
      for(int i = 0; i < producerList.size(); i++)
      {
         producerList.get(i).start();
      }
      for(int i = 0; i < consumerList.size(); i++)
      {
         consumerList.get(i).start();
      }
      
   }
   
   public static void main (String[] args)
   {
      new TestFilter();
   }
   
}

