#include <iostream>
#include <vector>
#include <mutex>
#include <condition_variable>

using namespace std;

class BankersAlgorithm
{
private:
    vector<vector<int>> allocation;
    vector<vector<int>> max;
    vector<int> available;
    vector<int> resources;
    vector<bool> completed;
    mutex mtx;
    condition_variable cv;

public:
    BankersAlgorithm(const vector<vector<int>> &allocation, const vector<vector<int>> &max,
                     const vector<int> &available) : allocation(allocation), max(max), available(available)
    {
        int numProcesses = allocation.size();
        int numResources = allocation[0].size();
        completed.resize(numProcesses, false);
        resources.resize(numResources, 0);

        for (int i = 0; i < numProcesses; ++i)
        {
            for (int j = 0; j < numResources; ++j)
            {
                resources[j] += allocation[i][j];
            }
        }
    }

    bool requestResources(int processId, const vector<int> &request)
    {
        unique_lock<mutex> lock(mtx);

        // Check if the requested resources are available and within max claim
        for (int i = 0; i < request.size(); ++i)
        {
            if (request[i] > max[processId][i] - allocation[processId][i] || request[i] > available[i])
            {
                return false;
            }
        }

        // Temporarily allocate the requested resources
        vector<vector<int>> tempAllocation = allocation;
        vector<int> tempAvailable = available;
        vector<int> tempResources = resources;

        // Update temporary vectors with the requested resources
        for (int i = 0; i < request.size(); ++i)
        {
            tempAllocation[processId][i] += request[i];
            tempAvailable[i] -= request[i];
            tempResources[i] -= request[i];
        }

        // Create a temporary BankersAlgorithm object with the updated allocation and available resources
        BankersAlgorithm tempBanker(tempAllocation, max, tempAvailable);

        // Check if the state is safe
        bool safeState = tempBanker.isSafeState();

        if (safeState)
        {
            allocation = tempAllocation;
            available = tempAvailable;
            resources = tempResources;
            completed[processId] = true;

            // Add the resources back to available and resources after the process has completed
            for (int i = 0; i < request.size(); ++i)
            {
                available[i] += request[i];
                resources[i] += request[i];
            }

            return true;
        }
        else
        {
            return false;
        }
    }

    void releaseResources(int processId, const vector<int> &release)
    {
        unique_lock<mutex> lock(mtx);

        // Release the resources
        for (int i = 0; i < release.size(); ++i)
        {
            allocation[processId][i] -= release[i];
            available[i] += release[i];
            resources[i] += release[i];
        }

        completed[processId] = false;
        cv.notify_all();
    }

    bool isSafeState()
    {
        int numProcesses = allocation.size();
        int numResources = allocation[0].size();

        vector<bool> safe(numProcesses, false);
        vector<vector<int>> need(numProcesses, vector<int>(numResources, 0));
        vector<int> work = available;

        // Calculate the need matrix
        for (int i = 0; i < numProcesses; ++i)
        {
            for (int j = 0; j < numResources; ++j)
            {
                need[i][j] = max[i][j] - allocation[i][j];
            }
        }

        bool finished;
        int count = 0;

        do
        {
            finished = true;

            for (int i = 0; i < numProcesses; ++i)
            {
                if (safe[i])
                    continue;

                bool canExecute = true;

                for (int j = 0; j < numResources; ++j)
                {
                    if (need[i][j] > work[j])
                    {
                        canExecute = false;
                        break;
                    }
                }

                if (canExecute)
                {
                    safe[i] = true;
                    for (int j = 0; j < numResources; ++j)
                    {
                        work[j] += allocation[i][j];
                    }
                    finished = false;
                    count++;
                }
            }
        } while (!finished && count < numProcesses);

        if (count < numProcesses)
        {
            return false; // Deadlock detected
        }

        for (bool isSafe : safe)
        {
            if (!isSafe)
                return false;
        }

        return true;
    }

    void printAllocation()
    {
        cout << "Allocation Matrix:\n";
        for (const auto &row : allocation)
        {
            for (int value : row)
                cout << value << " ";
            cout << "\n";
        }
    }

    void printMax()
    {
        cout << "Max Matrix:\n";
        for (const auto &row : max)
        {
            for (int value : row)
                cout << value << " ";
            cout << "\n";
        }
    }

    void printAvailable()
    {
        cout << "Available Resources: ";
        for (int value : available)
            cout << value << " ";
        cout << "\n";
    }

    void printResources()
    {
        cout << "Total Resources: ";
        for (int value : resources)
            cout << value << " ";
        cout << "\n";
    }
};

void runScenarios()
{
    vector<vector<int>> allocation = {
        {0, 1, 0},
        {2, 0, 0},
        {3, 0, 2},
        {2, 1, 1},
        {0, 0, 2}};

    vector<vector<int>> max = {
        {7, 5, 3},
        {3, 2, 2},
        {9, 0, 2},
        {2, 2, 2},
        {4, 3, 3}};

    vector<int> available = {3, 3, 2};
    BankersAlgorithm bankers(allocation, max, available);

    // Scenario 1: Successful resource request
    int processId = 1;
    vector<int> request = {1, 0, 2};
    bool requestSuccess = bankers.requestResources(processId, request);
    cout << "Scenario 1: Resource request for process " << processId << " -> " << (requestSuccess ? "Success" : "Failure") << "\n";

    // Scenario 2: Unsuccessful resource request (exceeds available resources)
    processId = 0;
    request = {4, 3, 1};
    requestSuccess = bankers.requestResources(processId, request);
    cout << "Scenario 2: Resource request for process " << processId << " -> " << (requestSuccess ? "Success" : "Failure") << "\n";

    // Scenario 3: Unsuccessful resource request (exceeds max claim)
    processId = 2;
    request = {6, 0, 0};
    requestSuccess = bankers.requestResources(processId, request);
    cout << "Scenario 3: Resource request for process " << processId << " -> " << (requestSuccess ? "Success" : "Failure") << "\n";

    // Scenario 4: Resource release
    processId = 1;
    vector<int> release = {3, 0, 2};
    bankers.releaseResources(processId, release);
    cout << "Scenario 4: Resource release for process " << processId << "\n";

    bankers.printAllocation();
    bankers.printAvailable();
    bankers.printResources();
}

void runScenarios2()
{
    vector<vector<int>> allocation = {
        {0, 1, 0},
        {2, 0, 0},
        {3, 2, 1},
        {2, 1, 1},
        {0, 0, 2}};

    vector<vector<int>> max = {
        {7, 5, 3},
        {3, 2, 2},
        {5, 4, 3},
        {4, 3, 3},
        {6, 5, 4}};

    vector<int> available = {3, 3, 2};
    BankersAlgorithm bankers(allocation, max, available);

    // Scenario 1: Successful resource request
    int processId = 1;
    vector<int> request = {1, 0, 2};
    bool requestSuccess = bankers.requestResources(processId, request);
    cout << "Scenario 1: Resource request for process " << processId << " -> " << (requestSuccess ? "Success" : "Failure") << "\n";

    // Scenario 2: Unsuccessful resource request (exceeds available resources)
    processId = 0;
    request = {4, 3, 1};
    requestSuccess = bankers.requestResources(processId, request);
    cout << "Scenario 2: Resource request for process " << processId << " -> " << (requestSuccess ? "Success" : "Failure") << "\n";

    // Scenario 3: Unsuccessful resource request (exceeds max claim)
    processId = 2;
    request = {6, 0, 0};
    requestSuccess = bankers.requestResources(processId, request);
    cout << "Scenario 3: Resource request for process " << processId << " -> " << (requestSuccess ? "Success" : "Failure") << "\n";

    // Scenario 4: Resource release
    processId = 1;
    vector<int> release = {3, 0, 2};
    bankers.releaseResources(processId, release);
    cout << "Scenario 4: Resource release for process " << processId << "\n";

    bankers.printAllocation();
    bankers.printAvailable();
    bankers.printResources();
}

int main()
{
    int choice;
    do
    {
        cout << "Select an option:\n";
        cout << "1. Run default scenarios\n";
        cout << "2. Run Secondary scenarios\n";
        cout << "3. Exit\n";
        cout << "Choice: ";
        cin >> choice;

        switch (choice)
        {
        case 1:
            runScenarios();
            break;
        case 2:
            runScenarios2();
            break;
        case 3:
            cout << "Exiting...\n";
            break;
        default:
            cout << "Invalid choice. Please try again.\n";
        }
    } while (choice != 3);

    return 0;
}
