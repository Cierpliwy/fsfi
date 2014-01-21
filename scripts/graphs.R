#!/usr/share/Rscript
# Scenario graphs rendered using R language and ggplot package.
library(ggplot2)
library(data.table)
library(reshape)
library(stringr)
library(grid)

readFile <- function(fileName) {
    out <- ""
    for(line in readLines(fileName, encoding="latin1"))
        out <- paste0(out, line, "\n")
    return(out)
}

createDataFrame <- function(rootPath) {
    # Data file
    mydata <- NULL

    # For each scenario...
    dirs <- list.dirs(rootPath)
    for (dir in dirs) {
        # Get scenario name
        dirName <- strsplit(dir,"/")[[1]]
        dirName <- dirName[length(dirName)]
        dirName2 <- gsub("_","\\\\_",dirName)
        print(dirName)

        # Visible variables
        iteration <- 0

        # Read every kernel log
        logs = list.files(dir, pattern="*_kernel.log")
        for (log in logs) {
            # Get iteration number
            iteration <- as.integer(strsplit(log,"_")[[1]][1])
            # Get files contents
            kernelOut <- readFile(paste0(dir,'/',log))
            programOut <- readFile(paste0(dir,'/',iteration,"_program.log"))

            # GET DATA
            # Any injection
            injection <- str_detect(kernelOut, "BITFLIP")

            # Is test successful
            successful <- str_detect(programOut, "TEST SUCCESSFUL")
            
            # Is test failed
            failed <- str_detect(programOut, "TEST FAILED")

            # Test completion status
            test_started <- str_detect(programOut, "Test started")
            test_finished <- str_detect(programOut, "Test finished")
            check_started <- str_detect(programOut, "Check started")
            check_finished <- str_detect(programOut, "Check finished")

            # Get signal
            signal <- str_match(programOut, "signal: ([^\n]+)")[2]

            # Get register
            reg <- str_match(kernelOut, "REG: ([^ ]+)")[2]

            # Get number of retires
            retries <- str_match_all(programOut, "Retrying... ([0-9]+)/")
            if (length(retries[[1]]) > 0) {
                retries <- retries[[1]][length(retries[[1]][,1]),2]
            } else {
                retries <- 0
            }

            # ADD ROWS
            rowData <- data.table(scenario=dirName2,
                                  iteration=iteration,
                                  injection=injection, 
                                  successful=successful, 
                                  failed=failed, 
                                  test_started=test_started, 
                                  test_finished=test_finished,
                                  check_started=check_started, 
                                  check_finished=check_finished,
                                  signal=signal,
                                  retries=retries,
                                  reg=reg)

            mydata <- rbind(mydata, rowData)
        }
    }

    return(mydata)
}

testResultPlot <- function(data)
{
    data2 <- melt(data, id=c("scenario"))
    ggplot(data2, aes(x=scenario, y=value, fill=variable))+ 
    scale_fill_manual(values=c("#00BA38", "#F8766D", "#619CFF"), 
                      name="Wynik testu",
                      breaks=c("successful", "failure", "other"),
                      labels=c("Powodzenie", "Błąd", "Nieokreślony")) +
    geom_bar(stat="identity") +
    xlab("Scenariusz") +
    ylab("Liczba testów (\\%)") +
    theme(axis.text.x=element_text(angle = -45, hjust = 0))
}

testResultPlot2 <- function(data)
{
    codeSuccess <- mean(data[regexpr("-",scenario) != -1, successful])
    codeFailure <- mean(data[regexpr("-",scenario) != -1, failure])
    codeOther <- 100 - codeSuccess - codeFailure

    regSuccess<- mean(data[regexpr("_",scenario) != -1, successful])
    regFailure <- mean(data[regexpr("_",scenario) != -1, failure])
    regOther <- 100 - regSuccess - regFailure

    data <- data.table(scenario = "Dane statyczne", suc=codeSuccess,
                       fail = codeFailure, oth = codeOther)
    data <- rbind(data, data.table(scenario = "Rejestry", suc=regSuccess,
                                   fail = regFailure, oth=regOther))
    data <- melt(data, id=c("scenario"))
    print(data)
    ggplot(data, aes(x=scenario, y=value, fill=variable))+ 
    scale_fill_manual(values=c("#00BA38", "#F8766D", "#619CFF"), 
                      name="Wynik testu",
                      breaks=c("suc", "fail", "oth"),
                      labels=c("Powodzenie", "Błąd", "Nieokreślony")) +
    geom_bar(stat="identity", position = "dodge") +
    geom_text(aes(label=sprintf("%2.2f \\%%",value), y=value+2), position = position_dodge(width=0.9)) +
    xlab("Rodzaj wstrzyknięcia") +
    ylab("Liczba testów (\\%)")
}

testResultData <- function(data)
{
    scenarios <- unique(data[injection==TRUE,scenario])
    outData <- NULL
    for (s in scenarios) {
        i <- nrow(data[scenario==s,]) 
        success <- sum(data[scenario==s,successful]) * 100 / i
        failed <- sum(data[scenario==s,failed]) * 100 / i
        rest <- (100 - success - failed)
        outData <- rbind(outData, data.table(scenario=s, successful=success,
                                             failure=failed, other=rest))
    }

    return(outData)
}

signalPlot <- function(data)
{
    d <- data[check_finished == FALSE, signal]
    d <- count(d)
    ggplot(d, aes(x=x, y=freq)) + 
    geom_bar(stat="identity") +
    geom_text(aes(label=freq, y=freq + 100)) +
    xlab("Sygnał") +
    ylab("Liczba testów")
}

retriesPlot <- function(data)
{
    d <- data.table(count(data[injection==TRUE,list(retries, successful, failed)]))
    d2 <- NULL
    for(i in 0:max(d[,retries])) {
        total <- sum(d[retries==i,freq])
        succ <- d[retries==i & successful == TRUE, freq]
        fail <- d[retries==i & failed == TRUE, freq]

        if(length(succ) == 0) succ <- 0
        if(length(fail) == 0) fail <- 0
        othe <- total - succ - fail

        d2 <- rbind(d2, data.table(r=i, succ=succ/total*100, fail=fail/total*100,
                                        othe=othe/total*100))
    }
    print(d2)
    d <- melt(d2, id=c("r"))
    ggplot(d, aes(x=r, y=value, fill=variable))+ 
    scale_fill_manual(values=c("#00BA38", "#F8766D", "#619CFF"), 
                      name="Wynik testu",
                      breaks=c("succ", "fail", "othe"),
                      labels=c("Powodzenie", "Błąd", "Nieokreślony")) +
    geom_bar(stat="identity") +
    xlab("Ilość wznowień testu") +
    ylab("Liczba testów (\\%)")
}

regsPlot <- function(data)
{
    successCount <- count(data[!is.na(reg) & successful, list(reg)])
    failCount <- count(data[!is.na(reg) & failed, list(reg)])
    totalCount <- count(data[!is.na(reg), list(reg)])
    m1 <- merge(successCount, failCount, by=c("reg"), all=TRUE)
    m2 <- merge(m1, totalCount, by=c("reg"), all=TRUE)
    m2[is.na(m2)] <- 0
    df <- data.table(m2)
    df[, freq := freq - freq.x - freq.y]
    df <- data.table(melt(df, id=c("reg")))
    df[, pos := value/2]
    df[variable=="freq.y", pos := pos + df[reg==reg & variable=="freq.x", value]]
    df[variable=="freq", pos := pos + df[reg==reg & variable=="freq.x", value]
                                    + df[reg==reg & variable=="freq.y", value]]
    df[value==0, value := NA]
    df[reg=="ORIG_RAX", reg := "ORIG\\_RAX"]
    print(df)
    ggplot(df, aes(x=reg, y=value, fill=variable))+ 
    scale_fill_manual(values=c("#00BA38", "#F8766D", "#619CFF"), 
                      name="Wynik testu",
                      breaks=c("freq.x", "freq.y", "freq"),
                      labels=c("Powodzenie", "Błąd", "Nieokreślony")) +
    geom_bar(stat="identity") +
    xlab("Zakłócany rejestr") +
    ylab("Liczba testów") +
    geom_text(aes(label=value, y=pos), size=3) +
    theme(axis.text.x=element_text(angle = -45, hjust = 0))
}
