import java.util.ArrayList;
import java.util.List;

class WorkerConnectionList
{
    List<WorkerConnection> connectionList;
    List<VdmsTransaction> resultList;
    
    public WorkerConnectionList()
    {
	connectionList = new ArrayList();
	resultList = new ArrayList();
    }

    public void AddWorker(WorkerConnection nWorker)
    {
	connectionList.add(nWorker);
    }

    public WorkerConnection Get(int i)
    {
	return connectionList.get(i);
    }

    public int GetSize()
    {
	return connectionList.size();
    }

    public VdmsTransaction Publish(VdmsTransaction outMessage)
    {
	VdmsTransaction returnValue = null;
	for(int i = 0; i < connectionList.size(); i++)
	    {
		returnValue = connectionList.get(i).Publish(outMessage);
	    }
	return returnValue;
    }

    public void ClearResults()
    {
	resultList.clear();
    }


}
