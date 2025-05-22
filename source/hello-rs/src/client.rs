use hello_world::greeter_client::GreeterClient;
use hello_world::HelloRequest;
use clap::{arg, value_parser, command};

pub mod hello_world {
    tonic::include_proto!("helloworld");
}

#[tokio::main]
async fn main() -> Result<(), Box<dyn std::error::Error>> {
    let matches = command!() // requires `cargo` feature
        .arg(arg!([name] "Optional name to operate on"))
        .arg(
            arg!(
                -s <server> "设置服务地址"
            )
            .required(true)
            .value_parser(value_parser!(String)),
        )
        .get_matches();

    let server = matches.get_one::<String>("server").ok_or("Server argument is required")?;
    
    let mut client = GreeterClient::connect(server.to_string()).await?;

    let request = tonic::Request::new(HelloRequest {
        name: "Tonic".into(),
    });

    let response = client.say_hello(request).await?;

    println!("RESPONSE={:?}", response);

    Ok(())
}