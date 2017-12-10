package ru.spbu.pt.bd.grpc

import io.grpc.ManagedChannelBuilder
import io.grpc.ServerBuilder
import io.grpc.stub.StreamObserver
import java.util.*

fun factorize_number(n: Int):List<Int> {
    var tmp_n = n
    var res: MutableList<Int> = mutableListOf()
    var d = 2
    while (d * d <= tmp_n) {
        if(tmp_n % d == 0) {
            res.add(d)
            tmp_n /= d
        } else {
            d += 1
        }
    }

    if(tmp_n > 1) {
        res.add(tmp_n)
    }
    return res.distinct()
}

class SampleService(): SampleServiceGrpc.SampleServiceImplBase() {

    override fun factorize(request: FactorRequest, responseObserver: StreamObserver<FactorResponse>) {
        println(request.number)
        val factors = factorize_number(request.number)
        val response = FactorResponse.newBuilder()
        factors.forEach{i -> response.addFactors(i)}
        val response_b = response.build()

        responseObserver.onNext(response_b)
        responseObserver.onCompleted()
    }
}


fun main(args: Array<String>) {
    val server = ServerBuilder
            .forPort(5757)
            .addService(SampleService())
            .build()
    server.start()

    println("Simple grpc service started")

    server.awaitTermination()

    println("Simple grpc service stopped")
}