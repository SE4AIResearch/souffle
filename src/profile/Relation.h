/*
 * Souffle - A Datalog Compiler
 * Copyright (c) 2016, The Souffle Developers. All rights reserved
 * Licensed under the Universal Permissive License v 1.0 as shown at:
 * - https://opensource.org/licenses/UPL
 * - <souffle root>/licenses/SOUFFLE-UPL.txt
 */

#pragma once

#include "Iteration.h"
#include "Rule.h"
#include <chrono>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace souffle {
namespace profile {

/*
 * Stores the iterations and rules of a given relation
 */
class Relation {
private:
    const std::string name;
    std::chrono::microseconds starttime{};
    std::chrono::microseconds endtime{};
    std::chrono::microseconds loadtime{};
    std::chrono::microseconds savetime{};
    long nonRecTuples = 0;
    size_t preMaxRSS = 0;
    size_t postMaxRSS = 0;
    const std::string id;
    std::string locator;
    int ruleId = 0;
    int recursiveId = 0;
    size_t tuplesRead = 0;

    std::vector<std::shared_ptr<Iteration>> iterations;

    std::unordered_map<std::string, std::shared_ptr<Rule>> ruleMap;

    bool ready = true;

public:
    Relation(std::string name, std::string id) : name(std::move(name)), id(std::move(id)) {
        ruleMap = std::unordered_map<std::string, std::shared_ptr<Rule>>();
        iterations = std::vector<std::shared_ptr<Iteration>>();
    }

    std::string createID() {
        return "N" + id.substr(1) + "." + std::to_string(++ruleId);
    }

    std::string createRecID(std::string name) {
        for (auto& iter : iterations) {
            for (auto& rul : iter->getRules()) {
                if (rul.second->getName() == name) {
                    return rul.second->getId();
                }
            }
        }
        return "C" + id.substr(1) + "." + std::to_string(++recursiveId);
    }

    std::chrono::microseconds getLoadtime() const {
        return loadtime;
    }

    std::chrono::microseconds getSavetime() const {
        return savetime;
    }

    std::chrono::microseconds getStarttime() const {
        return starttime;
    }

    std::chrono::microseconds getEndtime() const {
        return endtime;
    }

    std::chrono::microseconds getNonRecTime() const {
        return endtime - starttime;
    }

    std::chrono::microseconds getRecTime() const {
        std::chrono::microseconds result{};
        for (auto& iter : iterations) {
            result += iter->getRuntime();
        }
        return result;
    }

    std::chrono::microseconds getCopyTime() const {
        std::chrono::microseconds result{};
        for (auto& iter : iterations) {
            result += iter->getCopytime();
        }
        return result;
    }

    size_t size() const {
        size_t result = 0;
        for (auto& iter : iterations) {
            result += iter->size();
        }
        return nonRecTuples + result;
    }

    size_t getMaxRSSDiff() const {
        return postMaxRSS - preMaxRSS;
    }

    size_t getTotalRecursiveRuleSize() const {
        size_t result = 0;
        for (auto& iter : iterations) {
            for (auto& rul : iter->getRules()) {
                result += rul.second->size();
            }
        }
        return result;
    }

    void setLoadtime(std::chrono::microseconds loadtime) {
        this->loadtime = loadtime;
    }

    void setSavetime(std::chrono::microseconds savetime) {
        this->savetime = savetime;
    }

    void setStarttime(std::chrono::microseconds time) {
        starttime = time;
    }

    void setEndtime(std::chrono::microseconds time) {
        endtime = time;
    }

    void setNumTuples(long numTuples) {
        nonRecTuples = numTuples;
    }

    void setPostMaxRSS(size_t maxRSS) {
        postMaxRSS = std::max(maxRSS, postMaxRSS);
    }

    void setPreMaxRSS(size_t maxRSS) {
        if (preMaxRSS == 0) {
            preMaxRSS = maxRSS;
            return;
        }
        preMaxRSS = std::min(maxRSS, preMaxRSS);
    }

    std::string toString() const {
        std::ostringstream output;
        output << "{\n\"" << name << "\":[" << getNonRecTime().count() << "," << nonRecTuples
               << "],\n\n\"onRecRules\":[\n";
        for (auto& rul : ruleMap) {
            output << rul.second->toString();
        }
        output << "\n],\n\"iterations\":\n";
        output << "[";
        if (iterations.empty()) {
            output << ", ";
        }
        for (auto& iter : iterations) {
            output << iter->toString();
            output << ", ";
        }
        std::string retStr = output.str();
        // substring to remove the last comma
        return retStr.substr(0, retStr.size() - 2) + "]\n}";
    }

    std::string getName() const {
        return name;
    }

    /**
     * Return a map of Rules, indexed by srcLocator.
     *
     * @return the ruleMap
     */
    const std::unordered_map<std::string, std::shared_ptr<Rule>>& getRuleMap() const {
        return ruleMap;
    }

    void addRule(std::shared_ptr<Rule> rule) {
        ruleMap[rule->getLocator()] = rule;
    }

    std::vector<std::shared_ptr<Rule>> getRuleRecList() const {
        std::vector<std::shared_ptr<Rule>> temp = std::vector<std::shared_ptr<Rule>>();
        for (auto& iter : iterations) {
            for (auto& rul : iter->getRules()) {
                temp.push_back(rul.second);
            }
        }
        return temp;
    }

    const std::vector<std::shared_ptr<Iteration>>& getIterations() const {
        return iterations;
    }

    void addIteration(std::shared_ptr<Iteration> iteration) {
        iterations.push_back(iteration);
        if (endtime < iteration->getEndtime()) {
            endtime = iteration->getEndtime();
        }
        if (starttime.count() == 0 || starttime > iteration->getStarttime()) {
            starttime = iteration->getStarttime();
        }
    }

    const std::string& getId() const {
        return id;
    }

    const std::string& getLocator() const {
        return locator;
    }

    void setLocator(std::string locator) {
        this->locator = locator;
    }

    bool isReady() {
        return ready;
    }

    void setReady(bool ready) {
        this->ready = ready;
    }

    size_t getReads() const {
        return tuplesRead;
    }

    void addReads(size_t tuplesRead) {
        this->tuplesRead += tuplesRead;
    }
};

}  // namespace profile
}  // namespace souffle
