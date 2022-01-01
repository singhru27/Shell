## Project Name & Description


Collaborative grocery list application deployed on AWS, and implemented using Mongo Atlas, Express, Node, and Bootstrap. This application allows users to add, edit, and update a collaborative grocery product list in real time - and these changes are immediately reflected to all other users of the application. The Express server accepts put, post, get, and delete requests from clients and also features product filtering capabilities. All front end UI is built exclusively with CSS. EJS is used as a templating engine to produce easily reproducible HTML pages that can be flexibly applied with minimal code duplication. Mongoose is used as a driver to connect to the cloud-deployed Mongo Atlas database. The entire application is deployed on a single EC2 instance.

## Project Status

This project is completed

## Project Screen Shot(s)

#### Example:   

![ScreenShot](https://raw.github.com/singhru27/Cloud-Grocery-List/main/screenshots/Home.png)

## Installation and Setup Instructions

To see the application in action, visit the following URL. Please note that any data you submit will be visible to all other users of the application:
```
productdb.rujulsingh.com
```

To run the application locally/make changes, please do the following. 


Save the entire zip file onto your computer. Install the required dependencies by running 
```
npm install
```
The above requires that Node and NPM be installed on your computer. I recommend using Homebrew to set this up if you do not have it set up already. 


Following installation, change the Mongoose database connect call in line 19 of index.js (see below)

```
mongoose.connect(`mongodb+srv://singhru:${password}@rsdb.bodim.mongodb.net/Grocery?retryWrites=true&w=majority`)
    .then(() => {
        console.log("Connection Accepted");
    })
    .catch(() => {
        console.log("Connection Refused");
    });
```

 to point towards your own locally installed Mongo database or to a Mongo Atlas cluster. If you don't have Mongo installed/need to set up Mongo Atlas, please visit the following for a local Mongo installation
 ```
 https://docs.mongodb.com/manual/installation/
 ```
 
 or the following to set up Atlas
 
 ```
 https://docs.atlas.mongodb.com/getting-started/
 ```

Then run 
```
node index.ks
```
to run the server. 

The homepage can then be reached via browser at 
```
http://localhost:3000/
```