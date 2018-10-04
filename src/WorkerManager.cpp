#include "WorkerManager.h"
#include "CCBot.h"
#include "Util.h"
#include "Building.h"

WorkerManager::WorkerManager(CCBot & bot)
    : m_bot         (bot)
    , m_workerData  (bot)
{

}

void WorkerManager::onStart()
{
}

void WorkerManager::onFrame()
{
    m_workerData.updateAllWorkerData();
    handleGasWorkers();
    handleIdleWorkers();
	handleMineralWorkers();

    drawResourceDebugInfo();
    drawWorkerInformation();

    m_workerData.drawDepotDebugInfo();

    handleRepairWorkers();
}

void WorkerManager::setRepairWorker(Unit worker, const Unit & unitToRepair)
{
    m_workerData.setWorkerJob(worker, WorkerJobs::Repair, unitToRepair);
}

void WorkerManager::stopRepairing(Unit worker)
{
    m_workerData.setWorkerJob(worker, WorkerJobs::Idle);
}

void WorkerManager::handleGasWorkers()
{
    // for each unit we have
    for (auto & unit : m_bot.UnitInfo().getUnits(Players::Self))
    {
        // if that unit is a refinery
        if (unit.getType().isRefinery() && unit.isCompleted())
        {
            // get the number of workers currently assigned to it
            int numAssigned = m_workerData.getNumAssignedWorkers(unit);

            // if it's less than we want it to be, fill 'er up
            for (int i=0; i<(3-numAssigned); ++i)
            {
                auto gasWorker = getGasWorker(unit);
                if (gasWorker.isValid())
                {
                    m_workerData.setWorkerJob(gasWorker, WorkerJobs::Gas, unit);
                }
            }
        }
    }
}

void WorkerManager::handleIdleWorkers()
{

    // for each of our workers
    for (auto & worker : m_workerData.getWorkers())
    {
        if (!worker.isValid()) { continue; }

        bool isIdle = worker.isIdle();
        if (worker.isIdle() && 
			(m_workerData.getWorkerJob(worker) != WorkerJobs::Build) && 
			(m_workerData.getWorkerJob(worker) != WorkerJobs::Move) &&
			(m_workerData.getWorkerJob(worker) != WorkerJobs::Scout)) 
		{
			m_workerData.setWorkerJob(worker, WorkerJobs::Idle);
		}

        // if it is idle
        if (m_workerData.getWorkerJob(worker) == WorkerJobs::Idle)
        {
            setMineralWorker(worker);


			

        }
    }
}

void WorkerManager::handleRepairWorkers()
{
    // TODO
}

void WorkerManager::handleMineralWorkers()
{
	for (auto & worker : m_workerData.getWorkers())
	{
		if (!worker.isValid()) { continue; }

		if (m_workerData.getWorkerJob(worker) == WorkerJobs::Minerals) {


			const BaseLocation *workersBaseLocation;

			Unit depotWithWorkerSpace;
			bool possibleTransfer = false;
			Unit cc;

			//custom test, find every base, and find number of mineral workers for that base, as well as number of minerals.
			
			for (auto & base : m_bot.Bases().getBaseLocations()) {

				if (base->isOccupiedByPlayer(Players::Self)) {

					//std::cout << "Trying to find closest depot to a base from position x" << base->getPosition().x << " y" << base->getPosition().y << "\n";
					cc = getClosestDepot(base->getPosition());
					

					//std::cout << cc.getID() << " x:" << base->getDepotPosition().x << " y:" << base->getDepotPosition().y << " minerals:" << base->getMinerals().size() << ", with " << m_workerData.getNumAssignedWorkers(cc) << " workers\n";
					//find all bases that have mineral worker spaces
					
					if ((cc.getUnitPtr()->ideal_harvesters) > m_workerData.getNumAssignedWorkers(cc)) {
						
						//std::cout << "We have found a fresh base, " << cc.getID() << "\n";
						depotWithWorkerSpace = cc;
						possibleTransfer = true;

					}

					//if this is the current workers base
					if (m_workerData.getWorkerDepot(worker) == cc) {
						//save as his base location
						workersBaseLocation = base;
					}
				}
			}

			Unit workersDepot = m_workerData.getWorkerDepot(worker);

			//if current base is overflown, assign to one of the bases with empty spots.
			if ((workersDepot.getUnitPtr()->ideal_harvesters) < m_workerData.getNumAssignedWorkers(workersDepot)) {

				//std::cout << workersDepot.getID() << " Base is overflowing " << workersBaseLocation->getMinerals().size() * 2 << " minerals with " << m_workerData.getNumAssignedWorkers(workersDepot) << " workers \n";
				//and there is suitable base to transfer to

				//std::cout << depotWithWorkerSpace.isValid() << " validity \n";
				if (depotWithWorkerSpace.isValid() && possibleTransfer == true) {

					std::cout << "We have found a valid base to transfer! ";
					m_workerData.setWorkerJob(worker, WorkerJobs::Idle);
					m_workerData.setWorkerJob(worker, WorkerJobs::Minerals, depotWithWorkerSpace);
					break;
				}
			}

		}

	}
}

Unit WorkerManager::getClosestMineralWorkerTo(const CCPosition & pos) const
{
	//std::cout << "desired pos: x" << pos.x << " y" << pos.y << "\n";
    Unit closestMineralWorker;
    double closestDist = std::numeric_limits<double>::max();

	int count = 0;
    // for each of our workers
    for (auto & worker : m_workerData.getWorkers())
    {
        if (!worker.isValid()) { continue; }

		
        // if it is a mineral worker
        if (m_workerData.getWorkerJob(worker) == WorkerJobs::Minerals)
        {
			count++;
            double dist = Util::DistSq(worker.getPosition(), pos);

            if (!closestMineralWorker.isValid() || dist < closestDist)
            {
                closestMineralWorker = worker;
				closestDist = dist;
                //dist = closestDist;
            }
        }
    }

	//std::cout << "mineral workers: " << count << "\n";
	//std::cout << "closest wor: x" << (int)closestMineralWorker.getPosition().x << " y" << (int)closestMineralWorker.getPosition().y << "\n";
    return closestMineralWorker;
}


// set a worker to mine minerals
void WorkerManager::setMineralWorker(const Unit & unit)
{
    // check if there is a mineral available to send the worker to
    auto depot = getClosestDepot(unit.getPosition());

    // if there is a valid mineral
    if (depot.isValid()) {
    
        // update m_workerData with the new job
        m_workerData.setWorkerJob(unit, WorkerJobs::Minerals, depot);
    }
}

Unit WorkerManager::getClosestDepot(CCPosition unit) const
{
    Unit closestDepot;
    double closestDistance = std::numeric_limits<double>::max();

    for (auto & depot : m_bot.UnitInfo().getUnits(Players::Self))
    {
        if (!depot.isValid()) { continue; }
		
        if (depot.getType().isResourceDepot() && depot.isCompleted())
        {
			//std::cout << "registered depot " << unit.getID() << "\n"; //custom
			double distance = Util::Dist(depot.getPosition(), unit);
            if (!closestDepot.isValid() || distance < closestDistance)
            {
                closestDepot = depot;
                closestDistance = distance;
            }
        }
    }
	//std::cout << "closest depot " << closestDepot.getID() << " from pos x:" << unit.x << " y:" << unit.y << "\n";
    return closestDepot;
}


// other managers that need workers call this when they're done with a unit
void WorkerManager::finishedWithWorker(const Unit & unit)
{
    if (m_workerData.getWorkerJob(unit) != WorkerJobs::Scout)
    {
        m_workerData.setWorkerJob(unit, WorkerJobs::Idle);
    }
}

Unit WorkerManager::getGasWorker(Unit refinery) const
{
    return getClosestMineralWorkerTo(refinery.getPosition());
}

void WorkerManager::setBuildingWorker(Unit worker, Building & b)
{
    m_workerData.setWorkerJob(worker, WorkerJobs::Build, b.buildingUnit);
}

// gets a builder for BuildingManager to use
// if setJobAsBuilder is true (default), it will be flagged as a builder unit
// set 'setJobAsBuilder' to false if we just want to see which worker will build a building
Unit WorkerManager::getBuilder(Building & b, bool setJobAsBuilder) const
{
    Unit builderWorker = getClosestMineralWorkerTo(Util::GetPosition(b.finalPosition));

    // if the worker exists (one may not have been found in rare cases)
    if (builderWorker.isValid() && setJobAsBuilder)
    {
        m_workerData.setWorkerJob(builderWorker, WorkerJobs::Build, b.builderUnit);
    }

    return builderWorker;
}

// sets a worker as a scout
void WorkerManager::setScoutWorker(Unit workerTag)
{
    m_workerData.setWorkerJob(workerTag, WorkerJobs::Scout);
}

void WorkerManager::setCombatWorker(Unit workerTag)
{
    m_workerData.setWorkerJob(workerTag, WorkerJobs::Combat);
}

void WorkerManager::drawResourceDebugInfo()
{
    if (!m_bot.Config().DrawResourceInfo)
    {
        return;
    }

    for (auto & worker : m_workerData.getWorkers())
    {
        if (!worker.isValid()) { continue; }

        if (worker.isIdle())
        {
            m_bot.Map().drawText(worker.getPosition(), m_workerData.getJobCode(worker));
        }

        auto depot = m_workerData.getWorkerDepot(worker);
        if (depot.isValid())
        {
            m_bot.Map().drawLine(worker.getPosition(), depot.getPosition());
        }
    }
}

void WorkerManager::drawWorkerInformation()
{
    if (!m_bot.Config().DrawWorkerInfo)
    {
        return;
    }

    std::stringstream ss;
    ss << "Workers: " << m_workerData.getWorkers().size() << "\n";

    int yspace = 0;

    for (auto & worker : m_workerData.getWorkers())
    {
        ss << m_workerData.getJobCode(worker) << " " << worker.getID() << "\n";

        m_bot.Map().drawText(worker.getPosition(), m_workerData.getJobCode(worker));
    }

    m_bot.Map().drawTextScreen(0.75f, 0.2f, ss.str());
}

bool WorkerManager::isFree(Unit worker) const
{
    return m_workerData.getWorkerJob(worker) == WorkerJobs::Minerals || m_workerData.getWorkerJob(worker) == WorkerJobs::Idle;
}

bool WorkerManager::isWorkerScout(Unit worker) const
{
    return (m_workerData.getWorkerJob(worker) == WorkerJobs::Scout);
}

bool WorkerManager::isBuilder(Unit worker) const
{
    return (m_workerData.getWorkerJob(worker) == WorkerJobs::Build);
}

int WorkerManager::getNumMineralWorkers()
{
    return m_workerData.getWorkerJobCount(WorkerJobs::Minerals);
}

int WorkerManager::getNumGasWorkers()
{
    return m_workerData.getWorkerJobCount(WorkerJobs::Gas);

}
